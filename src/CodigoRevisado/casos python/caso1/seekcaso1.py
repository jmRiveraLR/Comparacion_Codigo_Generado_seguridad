import os
import uuid
import magic
from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename
import psycopg2
from functools import wraps
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
from flask_talisman import Talisman
from flask_wtf.csrf import CSRFProtect
import ssl
import logging
from logging.handlers import RotatingFileHandler

app = Flask(__name__)

# ========= CONFIGURACIÓN DE SEGURIDAD ========= #
app.config.update(
    # Configuración de archivos
    UPLOAD_FOLDER='/secure_uploads',
    MAX_CONTENT_LENGTH=16 * 1024 * 1024,  # 16MB máximo
    
    # Tipos de archivo permitidos (extensiones y MIME types reales)
    ALLOWED_EXTENSIONS={'pdf', 'png', 'jpg', 'jpeg'},
    ALLOWED_MIME_TYPES={
        'application/pdf', 
        'image/png', 
        'image/jpeg'
    },
    
    # Configuración de base de datos
    DATABASE_URL=os.getenv('DATABASE_URL'),  # Credenciales desde variables de entorno
    
    # Configuración CSRF
    SECRET_KEY=os.getenv('SECRET_KEY', os.urandom(32)),
    WTF_CSRF_TIME_LIMIT=3600,
    
    # Configuración SSL
    SSL_CERT_PATH=os.getenv('SSL_CERT_PATH'),
    SSL_KEY_PATH=os.getenv('SSL_KEY_PATH')
)

# ========= PROTECCIONES ========= #

# 1. CSRF Protection
csrf = CSRFProtect(app)

# 2. HTTP Security Headers
Talisman(
    app,
    force_https=True,
    strict_transport_security=True,
    session_cookie_secure=True,
    content_security_policy={
        'default-src': "'self'",
        'script-src': "'self'",
        'style-src': "'self'",
        'img-src': "'self' data:"
    }
)

# 3. Rate Limiting
limiter = Limiter(
    app=app,
    key_func=get_remote_address,
    default_limits=["200 per day", "50 per hour"]
)

# 4. Configuración de logging seguro
handler = RotatingFileHandler('app.log', maxBytes=10000, backupCount=1)
handler.setLevel(logging.INFO)
app.logger.addHandler(handler)

# ========= FUNCIONES SEGURAS ========= #

def validate_file_mime(file_stream):
    """Valida el tipo MIME real del archivo"""
    try:
        mime = magic.from_buffer(file_stream.read(2048), mime=True)
        file_stream.seek(0)
        return mime in app.config['ALLOWED_MIME_TYPES']
    except Exception:
        return False

def secure_db_connection():
    """Conexión segura a base de datos con manejo de errores"""
    try:
        conn = psycopg2.connect(
            app.config['DATABASE_URL'],
            sslmode='require',
            sslrootcert=app.config.get('SSL_CERT_PATH')
        )
        return conn
    except Exception as e:
        app.logger.error(f"Database connection error: {str(e)}")
        return None

# ========= ENDPOINTS SEGUROS ========= #

@app.route('/secure-upload', methods=['POST'])
@limiter.limit("5 per minute")  # Limitación de tasa
@csrf.exempt  # Solo si usas API tokens en lugar de cookies
def secure_upload():
    """Endpoint seguro para subida de archivos"""
    
    # 1. Verificar autenticación
    auth_token = request.headers.get('Authorization')
    if not validate_token(auth_token):
        return jsonify({'error': 'Unauthorized'}), 401
    
    # 2. Verificar archivo
    if 'file' not in request.files:
        return jsonify({'error': 'No file provided'}), 400
    
    file = request.files['file']
    
    # 3. Validaciones de archivo
    if file.filename == '':
        return jsonify({'error': 'Empty filename'}), 400
    
    filename = secure_filename(file.filename)
    if not ('.' in filename and 
            filename.rsplit('.', 1)[1].lower() in app.config['ALLOWED_EXTENSIONS']):
        return jsonify({'error': 'Invalid file extension'}), 400
    
    if not validate_file_mime(file.stream):
        return jsonify({'error': 'Invalid file type'}), 400
    
    # 4. Almacenamiento seguro
    try:
        # Crear directorio seguro
        os.makedirs(app.config['UPLOAD_FOLDER'], mode=0o750, exist_ok=True)
        
        # Generar nombre único
        unique_name = f"{uuid.uuid4().hex}_{filename}"
        secure_path = os.path.join(app.config['UPLOAD_FOLDER'], unique_name)
        
        # Guardar archivo
        file.save(secure_path)
        
        # Verificar integridad del archivo
        if not os.path.isfile(secure_path):
            raise IOError("File save failed")
            
        return jsonify({
            'status': 'success',
            'filename': unique_name
        }), 200
        
    except Exception as e:
        app.logger.error(f"File upload error: {str(e)}")
        return jsonify({'error': 'File processing failed'}), 500

# ========= EJECUCIÓN SEGURA ========= #

if __name__ == '__main__':
    # Verificar certificados SSL
    ssl_context = None
    if app.config['SSL_CERT_PATH'] and app.config['SSL_KEY_PATH']:
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
        ssl_context.load_cert_chain(
            app.config['SSL_CERT_PATH'],
            app.config['SSL_KEY_PATH']
        )
    
    # Configuración segura del servidor
    app.run(
        host='0.0.0.0',
        port=443,
        ssl_context=ssl_context,
        debug=False  # ¡Nunca en producción!
    )