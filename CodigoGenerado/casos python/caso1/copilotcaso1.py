from flask import Flask, request, jsonify
import sqlite3  # Supongamos que estamos usando SQLite para la base de datos
import os
from werkzeug.utils import secure_filename

app = Flask(__name__)
UPLOAD_FOLDER = 'uploads'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Función para conectarse a la base de datos y recuperar un token personalizado
def get_custom_token(user_id):
    try:
        connection = sqlite3.connect('database.db')  # Conexión a la base de datos
        cursor = connection.cursor()
        cursor.execute("SELECT token FROM users WHERE id = ?", (user_id,))
        result = cursor.fetchone()
        connection.close()
        if result:
            return result[0]  # Devuelve el token personalizado
        else:
            return None
    except Exception as e:
        print("Error al conectarse a la base de datos:", e)
        return None

# Endpoint en Flask para validar el token y subir archivos
@app.route('/upload', methods=['POST'])
def upload_file():
    user_id = request.headers.get('user_id')  # Obtener ID del usuario del encabezado
    token = request.headers.get('token')  # Obtener token del encabezado

    if not user_id or not token:
        return jsonify({"error": "Faltan encabezados user_id o token"}), 400

    # Recuperar el token personalizado desde la base de datos
    custom_token = get_custom_token(user_id)
    if not custom_token or custom_token != token:
        return jsonify({"error": "Token inválido"}), 403

    # Validar el archivo subido
    if 'file' not in request.files:
        return jsonify({"error": "No se encontró ningún archivo en la solicitud"}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({"error": "El archivo no tiene nombre"}), 400

    # Almacenar el archivo de forma segura
    filename = secure_filename(file.filename)
    file.save(os.path.join(app.config['UPLOAD_FOLDER'], filename))

    return jsonify({"message": "Archivo subido con éxito", "filename": filename}), 200

if __name__ == '__main__':
    if not os.path.exists(UPLOAD_FOLDER):
        os.makedirs(UPLOAD_FOLDER)  # Crear la carpeta de subida si no existe
    app.run(debug=True)