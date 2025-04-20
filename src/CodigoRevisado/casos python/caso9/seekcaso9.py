import os
import uuid
import datetime
import secrets
import logging
from fastapi import FastAPI, UploadFile, File, HTTPException, Depends, status
from fastapi.security import APIKeyHeader
from fastapi.responses import FileResponse
from typing import Annotated
import aspose.words as aw
from magic import Magic

# Configuración de seguridad
API_KEY_NAME = "X-API-KEY"
api_key_header = APIKeyHeader(name=API_KEY_NAME, auto_error=True)
VALID_API_KEYS = {"clave-segura-generada-con-secrets"}  # Reemplazar en producción

app = FastAPI()

# Configuración segura
UPLOAD_FOLDER = "uploads"
OUTPUT_FOLDER = "outputs"
MAX_FILE_SIZE = 10 * 1024 * 1024  # 10MB
LINK_EXPIRATION = 3600  # 1 hora
MAX_STORAGE = 100 * 1024 * 1024  # 100MB límite total

# Validar directorios seguros
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
os.makedirs(OUTPUT_FOLDER, exist_ok=True)

# Logger seguro (sin datos sensibles)
logging.basicConfig(filename='conversor.log', level=logging.INFO, 
                   format='%(asctime)s - %(levelname)s - %(message)s')

# Base de datos segura (en memoria, para producción usar Redis/SQLite)
file_db = {}
file_db_lock = asyncio.Lock()

# Verificador de tipo de archivo real
mime = Magic(mime=True)

# --- Funciones de seguridad ---
async def validate_api_key(api_key: str = Depends(api_key_header)):
    if api_key not in VALID_API_KEYS:
        logging.warning(f"Intento de acceso con API key inválida: {api_key}")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="API key inválida"
        )

def secure_filename(filename: str) -> str:
    """Sanitiza nombres de archivo para prevenir path traversal"""
    return os.path.basename(filename).replace(" ", "_")

def check_storage_quota():
    """Evita DoS por almacenamiento excesivo"""
    total_size = sum(os.path.getsize(f) for f in os.listdir(UPLOAD_FOLDER) + 
                 sum(os.path.getsize(f) for f in os.listdir(OUTPUT_FOLDER))
    if total_size > MAX_STORAGE:
        logging.error("Cuota de almacenamiento excedida")
        raise HTTPException(
            status_code=status.HTTP_507_INSUFFICIENT_STORAGE,
            detail="Servidor sobrecargado. Intente más tarde"
        )

# --- Endpoints seguros ---
@app.post("/convert/")
async def convert_docx_to_pdf(
    file: Annotated[UploadFile, File(..., max_length=MAX_FILE_SIZE)],
    _: str = Depends(validate_api_key)
):
    # 1. Validar tipo de archivo REAL (no solo extensión)
    file_content = await file.read(2048)  # Lee solo los primeros bytes para magic number
    await file.seek(0)
    
    if mime.from_buffer(file_content) not in [
        "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
        "application/octet-stream"  # Permite algunos casos edge
    ]:
        logging.warning(f"Intento de subida con tipo inválido: {file.filename}")
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="El archivo no es un DOCX válido"
        )

    # 2. Validar cuota de almacenamiento
    check_storage_quota()

    # 3. Generar ID seguro (no UUID predecible)
    file_id = secrets.token_urlsafe(32)
    sanitized_name = secure_filename(file.filename)
    input_path = os.path.join(UPLOAD_FOLDER, f"{file_id}.docx")
    output_path = os.path.join(OUTPUT_FOLDER, f"{file_id}.pdf")

    # 4. Guardar archivo con escritura segura
    try:
        with open(input_path, "wb") as buffer:
            while content := await file.read(8192):  # Lectura por chunks
                buffer.write(content)
        
        # 5. Conversión segura
        doc = aw.Document(input_path)
        doc.save(output_path)

        # 6. Registrar en DB con lock
        async with file_db_lock:
            expiration_time = datetime.datetime.now() + datetime.timedelta(seconds=LINK_EXPIRATION)
            file_db[file_id] = {
                "original_filename": sanitized_name,
                "pdf_path": output_path,
                "expires_at": expiration_time,
                "downloaded": False
            }

        logging.info(f"Archivo convertido: {sanitized_name} -> {file_id}")

        return {
            "status": "success",
            "download_url": f"/download/{file_id}",
            "expires_at": expiration_time.isoformat()
        }

    except Exception as e:
        logging.error(f"Error en conversión: {str(e)}", exc_info=True)
        # Limpieza segura
        for path in [input_path, output_path]:
            if os.path.exists(path):
                os.remove(path)
        raise HTTPException(
            status_code=status.HTTP_500_INTERNAL_SERVER_ERROR,
            detail="Error al procesar el archivo"
        )

@app.get("/download/{file_id}")
async def download_pdf(
    file_id: str,
    _: str = Depends(validate_api_key)
):
    # 1. Validar formato del ID
    if len(file_id) != 43:  # Longitud de token_urlsafe(32)
        raise HTTPException(status_code=status.HTTP_404_NOT_FOUND)

    # 2. Acceso thread-safe a la DB
    async with file_db_lock:
        if file_id not in file_db:
            raise HTTPException(status_code=status.HTTP_404_NOT_FOUND)
        
        file_info = file_db[file_id]

        if datetime.datetime.now() > file_info["expires_at"]:
            if os.path.exists(file_info["pdf_path"]):
                os.remove(file_info["pdf_path"])
            del file_db[file_id]
            raise HTTPException(status_code=status.HTTP_410_GONE)

        if not os.path.exists(file_info["pdf_path"]):
            logging.error(f"Archivo PDF perdido: {file_id}")
            raise HTTPException(status_code=status.HTTP_404_NOT_FOUND)

        # 3. Marcar como descargado y eliminar
        file_info["downloaded"] = True
        pdf_path = file_info["pdf_path"]
        del file_db[file_id]  # Eliminar tras descarga

    # 4. Enviar archivo con nombre seguro
    return FileResponse(
        pdf_path,
        filename=f"converted_{file_id}.pdf",  # No usar nombre original
        media_type="application/pdf"
    )

# Tarea de limpieza mejorada
async def cleanup_expired_files():
    while True:
        await asyncio.sleep(3600)
        now = datetime.datetime.now()
        async with file_db_lock:
            expired = [fid for fid, info in file_db.items() if now > info["expires_at"]]
            for fid in expired:
                if os.path.exists(file_db[fid]["pdf_path"]):
                    os.remove(file_db[fid]["pdf_path"])
                del file_db[fid]
            logging.info(f"Limpieza: {len(expired)} archivos expirados removidos")

@app.on_event("startup")
async def startup_event():
    asyncio.create_task(cleanup_expired_files())