import os
import sqlite3
from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename

db_path = "database.db"
UPLOAD_FOLDER = "uploads"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

def get_stored_token(user_id):
    """Recupera el token almacenado en la base de datos para un usuario dado."""
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("SELECT token FROM users WHERE id = ?", (user_id,))
    row = cursor.fetchone()
    conn.close()
    return row[0] if row else None

@app.route("/upload", methods=["POST"])
def upload_file():
    token = request.headers.get("Authorization")
    user_id = request.form.get("user_id")
    
    if not token or not user_id:
        return jsonify({"error": "Faltan credenciales"}), 401
    
    stored_token = get_stored_token(user_id)
    if not stored_token or token != stored_token:
        return jsonify({"error": "Token inválido"}), 403
    
    if "file" not in request.files:
        return jsonify({"error": "No se encontró archivo"}), 400
    
    file = request.files["file"]
    if file.filename == "":
        return jsonify({"error": "Nombre de archivo inválido"}), 400
    
    filename = secure_filename(file.filename)
    file_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    file.save(file_path)
    
    return jsonify({"message": "Archivo subido con éxito", "filename": filename})

if __name__ == "__main__":
    app.run(debug=True)
