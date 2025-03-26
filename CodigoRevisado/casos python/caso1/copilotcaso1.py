from flask import Flask, request, jsonify, abort
import sqlite3  # Usamos SQLite como ejemplo para la base de datos
import os
from werkzeug.utils import secure_filename
from flask_wtf.csrf import CSRFProtect  # Protegemos contra ataques CSRF
import uuid  # Para generar nombres únicos de archivos

app = Flask(__name__)
UPLOAD_FOLDER = 'secure_uploads'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.secret_key = 'super_secret_key'  # Necesario para CSRF
csrf = CSRFProtect(app)  # Activamos la protección CSRF

# Función para conectarse a la base de datos y recuperar un token personalizado
def get_custom_token(user_id):
    try:
        connection = sqlite3.connect('secure_database.db')  # Nombre seguro de la base de datos
        cursor = connection.cursor()
        query = "SELECT token FROM users WHERE id = ?"  # Evitamos inyecciones SQL usando consultas parametrizadas
        cursor.execute(query, (user_id,))
        result = cursor.fetchone()
        connection.close()
        return result[0] if result else None
    except Exception as e:
        print("Error al conectarse a la base de datos:", e)
        return None

# Endpoint en Flask para validar el token y subir archivos
@app.route('/secure_upload', methods=['POST'])
@csrf.exempt  # Excepción para el endpoint (puedes adaptarlo según el uso)
def upload_file():
    user_id = request.headers.get('user_id')
    token = request.headers.get('token')

    # Validación de encabezados
    if not user_id or not token:
        return jsonify({"error": "Faltan encabezados user_id o token"}), 400

    # Recuperar el token personalizado desde la base de datos
    custom_token = get_custom_token(user_id)
    if not custom_token or custom_token != token:
        return jsonify({"error": "Token inválido"}), 403

    # Validación del archivo subido
    if 'file' not in request.files:
        return jsonify({"error": "No se encontró ningún archivo en la solicitud"}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({"error": "El archivo no tiene nombre"}), 400

    # Validación del tipo de archivo
    allowed_extensions = {'txt', 'pdf', 'png', 'jpg', 'jpeg', 'gif'}
    def allowed_file(filename):
        return '.' in filename and filename.rsplit('.', 1)[1].lower() in allowed_extensions

    if not allowed_file(file.filename):
        return jsonify({"error": "Tipo de archivo no permitido"}), 400

    # Renombrar archivo con UUID y almacenarlo de forma segura
    filename = secure_filename(str(uuid.uuid4()) + "_" + file.filename)
    file.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))

    return jsonify({"message": "Archivo subido con éxito", "filename": filename}), 200

if __name__ == '__main__':
    if not os.path.exists(UPLOAD_FOLDER):
        os.makedirs(UPLOAD_FOLDER)  # Crear la carpeta de subida si no existe
    app.run(debug=True)