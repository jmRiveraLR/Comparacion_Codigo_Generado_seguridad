from fastapi import FastAPI, UploadFile, HTTPException
from fastapi.responses import FileResponse
from tempfile import NamedTemporaryFile
from uuid import uuid4
from pathlib import Path
from threading import Timer
import os

app = FastAPI()

# Diccionario para almacenar archivos y controlar la validez del enlace
files_storage = {}

@app.post("/convert/")
async def convert_docx_to_pdf(file: UploadFile):
    # Validar tipo de archivo y tamaño (10MB)
    if file.content_type != "application/vnd.openxmlformats-officedocument.wordprocessingml.document":
        raise HTTPException(status_code=400, detail="El archivo debe ser un DOCX.")
    if file.size > 10 * 1024 * 1024:
        raise HTTPException(status_code=400, detail="El archivo no debe exceder los 10MB.")

    try:
        # Crear archivo temporal para almacenamiento
        with NamedTemporaryFile(delete=False, suffix=".docx") as temp_docx:
            temp_docx.write(await file.read())
            temp_docx_path = temp_docx.name

        pdf_filename = f"{uuid4().hex}.pdf"
        pdf_filepath = Path(temp_docx_path).parent / pdf_filename
        
        # Convertir DOCX a PDF (implementación mínima para ejemplo)
        docx_to_pdf(temp_docx_path, pdf_filepath)

        # Guardar en storage temporal
        files_storage[pdf_filename] = str(pdf_filepath)

        # Borrar archivo después de 1 hora
        Timer(3600, lambda: clean_up_file(pdf_filename)).start()

        return {"download_url": f"/download/{pdf_filename}"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error procesando el archivo: {str(e)}")


@app.get("/download/{file_id}")
async def download_file(file_id: str):
    file_path = files_storage.get(file_id)
    if not file_path or not os.path.exists(file_path):
        raise HTTPException(status_code=404, detail="El archivo no está disponible o ha expirado.")

    return FileResponse(file_path)


def docx_to_pdf(docx_path, pdf_path):
    # Mínima conversión por demostración
    from reportlab.pdfgen import canvas
    c = canvas.Canvas(str(pdf_path))
    c.drawString(100, 750, "Este es un PDF de demostración para el archivo DOCX.")
    c.save()


def clean_up_file(file_id):
    file_path = files_storage.pop(file_id, None)
    if file_path and os.path.exists(file_path):
        os.remove(file_path)


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)