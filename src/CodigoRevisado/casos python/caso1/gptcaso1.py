import os
import hashlib
import werkzeug
from flask import Flask, request, jsonify, abort, session
from werkzeug.utils import secure_filename
from datetime import datetime, timedelta
import secrets

app = Flask(__name__)
app.secret_key = 'supersecretkey'  # Clave secreta para la sesión CSRF

# Configuraciones de subida
UPLOAD_FOLDER = 'uploads'
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif', 'pdf'}
MAX_CONTENT_LENGTH = 5 * 1024 * 1024  # Limitar a 5MB

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = MAX_CONTENT_LENGTH

# Simulación de base de datos de tokens (deberías usar una base de datos real)
tokens_db = {
    "user123": {"token": "valid_token_123", "expiry": datetime.utcnow() + timedelta(hours=1)}
}

# Función para verificar si un archivo tiene una extensión permitida
def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

# Función para obtener el token del usuario
def get_stored_token(user_id):
    # Simulación de recuperación de token desde una base de datos
    token_info = tokens_db.get(user_id)
    if token_info and token_info["expiry"] > datetime.utcnow():
        return token_info["token"]
    return None

# Endpoint para subida de archivos
@app.route('/upload', methods=['POST'])
def upload_file():
    # Verificar token CSRF
    csrf_token = request.form.get('csrf_token')
    if csrf_token != session.get('csrf_token'):
        abort(403, "CSRF token inválido")

    # Verificar el token de autenticación
    auth_token = request.headers.get('Authorization')
    user_id = request.form.get('user_id')  # Obtener user_id del formulario

    if not auth_token or get_stored_token(user_id) != auth_token:
        abort(401, "Token de autenticación inválido")

    # Verificar si se subió un archivo
    if 'file' not in request.files:
        abort(400, "No se encontró archivo")

    file = request.files['file']

    if file.filename == '':
        abort(400, "No se seleccionó ningún archivo")

    if file and allowed_file(file.filename):
        # Asegurarse de que el archivo no esté fuera del directorio de carga
        filename = secure_filename(file.filename)
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        
        # Prevenir path traversal (asegurarse de que no se suban archivos con rutas peligrosas)
        if ".." in filename or filename.startswith("/"):
            abort(400, "Ruta del archivo no permitida")

        # Guardar archivo de forma segura
        file.save(file_path)
        return jsonify({"message": "Archivo subido exitosamente", "filename": filename}), 200
    else:
        abort(400, "Tipo de archivo no permitido")

# Función para generar un token CSRF
def generate_csrf_token():
    token = secrets.token_urlsafe(16)
    session['csrf_token'] = token
    return token

# Endpoint para obtener el token CSRF
@app.route('/csrf_token', methods=['GET'])
def csrf_token():
    return jsonify({"csrf_token": generate_csrf_token()}), 200

if __name__ == '__main__':
    # Asegúrate de que el directorio de uploads exista
    if not os.path.exists(UPLOAD_FOLDER):
        os.makedirs(UPLOAD_FOLDER)
    app.run(ssl_context='adhoc')  # Activar HTTPS (en producción usa un certificado real)
