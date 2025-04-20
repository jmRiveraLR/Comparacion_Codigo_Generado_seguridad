import os
import shutil
import uuid
from fastapi import FastAPI, File, UploadFile, HTTPException, BackgroundTasks
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from starlette.requests import Request
from datetime import datetime, timedelta
from typing import Dict

UPLOAD_DIR = "uploads"
RESULT_DIR = "converted"
MAX_FILE_SIZE_MB = 10
TOKEN_EXPIRATION = timedelta(hours=1)

app = FastAPI()

# Para servir los archivos PDF temporalmente
app.mount("/downloads", StaticFiles(directory=RESULT_DIR), name="downloads")

# Diccionario en memoria para tokens temporales
tokens: Dict[str, Dict] = {}

os.makedirs(UPLOAD_DIR, exist_ok=True)
os.makedirs(RESULT_DIR, exist_ok=True)


def convert_docx_to_pdf(input_path: str, output_path: str):
    """Usa libreoffice para convertir DOCX a PDF"""
    command = f"libreoffice --headless --convert-to pdf --outdir {RESULT_DIR} {input_path}"
    os.system(command)
    converted_name = os.path.splitext(os.path.basename(input_path))[0] + ".pdf"
    return os.path.join(RESULT_DIR, converted_name)


@app.post("/convert")
async def convert_file(background_tasks: BackgroundTasks, file: UploadFile = File(...)):
    # Validación de tipo de archivo
    if not file.filename.endswith(".docx"):
        raise HTTPException(status_code=400, detail="Solo se permiten archivos DOCX.")

    # Validación de tamaño
    contents = await file.read()
    size_mb = len(contents) / (1024 * 1024)
    if size_mb > MAX_FILE_SIZE_MB:
        raise HTTPException(status_code=413, detail="Archivo excede el límite de 10 MB.")

    # Guardar archivo
    file_id = str(uuid.uuid4())
    file_path = os.path.join(UPLOAD_DIR, f"{file_id}.docx")
    with open(file_path, "wb") as f:
        f.write(contents)

    # Convertir a PDF
    pdf_path = convert_docx_to_pdf(file_path, RESULT_DIR)
    if not os.path.exists(pdf_path):
        raise HTTPException(status_code=500, detail="Error al convertir el archivo.")

    # Generar token
    token = str(uuid.uuid4())
    tokens[token] = {
        "path": pdf_path,
        "expires": datetime.utcnow() + TOKEN_EXPIRATION,
    }

    # Borrar archivo original DOCX
    background_tasks.add_task(os.remove, file_path)

    # Devolver enlace de descarga
    return {"download_url": f"/download/{token}"}


@app.get("/download/{token}")
def download_file(token: str):
    if token not in tokens:
        raise HTTPException(status_code=404, detail="Token inválido o expirado.")

    token_data = tokens[token]
    if datetime.utcnow() > token_data["expires"]:
        del tokens[token]
        raise HTTPException(status_code=410, detail="El enlace ha expirado.")

    return FileResponse(token_data["path"], media_type="application/pdf", filename=os.path.basename(token_data["path"]))


@app.on_event("startup")
def clean_expired_files():
    """Limpieza programada simple al reiniciar"""
    now = datetime.utcnow()
    expired = [token for token, data in tokens.items() if data["expires"] < now]
    for token in expired:
        try:
            os.remove(tokens[token]["path"])
        except:
            pass
        del tokens[token]
