import os
import uuid
import datetime
from fastapi import FastAPI, UploadFile, File, HTTPException
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
import aspose.words as aw

app = FastAPI()

# Configuración
UPLOAD_FOLDER = "uploads"
OUTPUT_FOLDER = "outputs"
MAX_FILE_SIZE = 10 * 1024 * 1024  # 10MB
LINK_EXPIRATION = 3600  # 1 hora en segundos

# Crear directorios si no existen
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# Diccionario para almacenar información de los archivos generados
file_db = {}

@app.post("/convert/")
async def convert_docx_to_pdf(file: UploadFile = File(...)):
    # Verificar tamaño del archivo
    file_size = 0
    for chunk in file.file:
        file_size += len(chunk)
        if file_size > MAX_FILE_SIZE:
            raise HTTPException(
                status_code=413,
                detail=f"El archivo es demasiado grande. Tamaño máximo permitido: {MAX_FILE_SIZE/1024/1024}MB"
            )
    
    # Verificar extensión del archivo
    if not file.filename.lower().endswith('.docx'):
        raise HTTPException(
            status_code=400,
            detail="Solo se aceptan archivos con extensión .docx"
        )
    
    # Generar nombres únicos para los archivos
    file_id = str(uuid.uuid4())
    input_path = os.path.join(UPLOAD_FOLDER, f"{file_id}.docx")
    output_path = os.path.join(OUTPUT_FOLDER, f"{file_id}.pdf")
    
    # Guardar el archivo subido
    with open(input_path, "wb") as buffer:
        file.file.seek(0)  # Volver al inicio del archivo después de verificar el tamaño
        buffer.write(file.file.read())
    
    try:
        # Convertir DOCX a PDF usando Aspose.Words
        doc = aw.Document(input_path)
        doc.save(output_path)
        
        # Registrar en la base de datos "ficticia"
        expiration_time = datetime.datetime.now() + datetime.timedelta(seconds=LINK_EXPIRATION)
        file_db[file_id] = {
            "original_filename": file.filename,
            "pdf_path": output_path,
            "expires_at": expiration_time,
            "downloaded": False
        }
        
        # Generar URL de descarga
        download_url = f"/download/{file_id}"
        
        return {
            "status": "success",
            "message": "Archivo convertido exitosamente",
            "download_url": download_url,
            "expires_at": expiration_time.isoformat(),
            "original_filename": file.filename
        }
    except Exception as e:
        # Limpiar archivos en caso de error
        if os.path.exists(input_path):
            os.remove(input_path)
        if os.path.exists(output_path):
            os.remove(output_path)
        raise HTTPException(
            status_code=500,
            detail=f"Error al convertir el archivo: {str(e)}"
        )

@app.get("/download/{file_id}")
async def download_pdf(file_id: str):
    # Verificar si el archivo existe en la base de datos
    if file_id not in file_db:
        raise HTTPException(status_code=404, detail="Archivo no encontrado")
    
    file_info = file_db[file_id]
    
    # Verificar si el enlace ha expirado
    if datetime.datetime.now() > file_info["expires_at"]:
        # Limpiar archivos expirados
        if os.path.exists(file_info["pdf_path"]):
            os.remove(file_info["pdf_path"])
        del file_db[file_id]
        raise HTTPException(status_code=410, detail="El enlace de descarga ha expirado")
    
    # Verificar si el archivo PDF existe
    if not os.path.exists(file_info["pdf_path"]):
        raise HTTPException(status_code=404, detail="Archivo PDF no encontrado")
    
    # Marcar como descargado (opcional)
    file_db[file_id]["downloaded"] = True
    
    # Devolver el archivo PDF
    return FileResponse(
        file_info["pdf_path"],
        filename=f"{os.path.splitext(file_info['original_filename'])[0]}.pdf",
        media_type="application/pdf"
    )

# Tarea de limpieza periódica para eliminar archivos expirados
async def cleanup_expired_files():
    while True:
        await asyncio.sleep(3600)  # Ejecutar cada hora
        now = datetime.datetime.now()
        expired_files = [file_id for file_id, info in file_db.items() if now > info["expires_at"]]
        
        for file_id in expired_files:
            file_info = file_db[file_id]
            if os.path.exists(file_info["pdf_path"]):
                os.remove(file_info["pdf_path"])
            del file_db[file_id]

# Iniciar tarea de limpieza al arrancar la aplicación
@app.on_event("startup")
async def startup_event():
    import asyncio
    asyncio.create_task(cleanup_expired_files())

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)