import os
import shutil
import uuid
import magic
import subprocess
from fastapi import FastAPI, File, UploadFile, HTTPException, BackgroundTasks
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from datetime import datetime, timedelta
from typing import Dict

UPLOAD_DIR = "uploads"
RESULT_DIR = "converted"
MAX_FILE_SIZE_MB = 10
TOKEN_EXPIRATION = timedelta(hours=1)

app = FastAPI()
app.mount("/downloads", StaticFiles(directory=RESULT_DIR), name="downloads")

tokens: Dict[str, Dict] = {}

os.makedirs(UPLOAD_DIR, exist_ok=True)
os.makedirs(RESULT_DIR, exist_ok=True)


def is_docx(file_path: str) -> bool:
    """Verifica que el archivo subido sea un DOCX real"""
    mime = magic.Magic(mime=True)
    return mime.from_file(file_path) == "application/vnd.openxmlformats-officedocument.wordprocessingml.document"


def convert_docx_to_pdf(input_path: str) -> str:
    """Convierte un archivo DOCX a PDF usando LibreOffice de forma segura"""
    try:
        subprocess.run(
            ["libreoffice", "--headless", "--convert-to", "pdf", "--outdir", RESULT_DIR, input_path],
            check=True,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
    except subprocess.CalledProcessError:
        raise HTTPException(status_code=500, detail="Error en la conversión de LibreOffice")

    pdf_name = os.path.splitext(os.path.basename(input_path))[0] + ".pdf"
    return os.path.join(RESULT_DIR, pdf_name)


@app.post("/convert")
async def convert_file(background_tasks: BackgroundTasks, file: UploadFile = File(...)):
    if not file.filename.endswith(".docx"):
        raise HTTPException(status_code=400, detail="Solo se permiten archivos DOCX.")

    contents = await file.read()
    if len(contents) > MAX_FILE_SIZE_MB * 1024 * 1024:
        raise HTTPException(status_code=413, detail="Archivo excede el límite de 10 MB.")

    file_id = str(uuid.uuid4())
    temp_path = os.path.join(UPLOAD_DIR, f"{file_id}.docx")
    with open(temp_path, "wb") as f:
        f.write(contents)

    if not is_docx(temp_path):
        os.remove(temp_path)
        raise HTTPException(status_code=400, detail="El archivo no es un DOCX válido.")

    pdf_path = convert_docx_to_pdf(temp_path)
    if not os.path.exists(pdf_path):
        raise HTTPException(status_code=500, detail="No se generó el archivo PDF.")

    token = str(uuid.uuid4())
    tokens[token] = {
        "path": pdf_path,
        "expires": datetime.utcnow() + TOKEN_EXPIRATION
    }

    background_tasks.add_task(os.remove, temp_path)

    return {"download_url": f"/download/{token}"}


@app.get("/download/{token}")
def download_file(token: str, background_tasks: BackgroundTasks):
    token_data = tokens.get(token)
    if not token_data:
        raise HTTPException(status_code=404, detail="Token inválido o expirado.")

    if datetime.utcnow() > token_data["expires"]:
        try:
            os.remove(token_data["path"])
        except FileNotFoundError:
            pass
        del tokens[token]
        raise HTTPException(status_code=410, detail="El enlace ha expirado.")

    background_tasks.add_task(delayed_cleanup, token)
    return FileResponse(token_data["path"], media_type="application/pdf", filename=os.path.basename(token_data["path"]))


def delayed_cleanup(token: str):
    """Limpieza tras descarga o expiración"""
    token_data = tokens.get(token)
    if token_data:
        try:
            os.remove(token_data["path"])
        except FileNotFoundError:
            pass
        tokens.pop(token, None)
