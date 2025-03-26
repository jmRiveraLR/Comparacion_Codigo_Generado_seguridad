import os
import uuid
from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename
import psycopg2  # Para PostgreSQL, puedes cambiar según tu BD
from functools import wraps

app = Flask(__name__)

# Configuración de la aplicación
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['ALLOWED_EXTENSIONS'] = {'txt', 'pdf', 'png', 'jpg', 'jpeg', 'gif'}
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB máximo

# Configuración de la base de datos (ajusta según tu entorno)
DB_CONFIG = {
    'host': 'tu_host_de_bd',
    'database': 'nombre_bd',
    'user': 'usuario_bd',
    'password': 'contraseña_bd'
}

def get_db_connection():
    """Establece conexión con la base de datos externa"""
    try:
        conn = psycopg2.connect(**DB_CONFIG)
        return conn
    except Exception as e:
        print(f"Error al conectar a la BD: {e}")
        return None

def get_custom_token_from_db(user_id):
    """Recupera un token personalizado de la base de datos para un usuario"""
    conn = get_db_connection()
    if not conn:
        return None
    
    try:
        with conn.cursor() as cursor:
            cursor.execute("SELECT token FROM user_tokens WHERE user_id = %s", (user_id,))
            result = cursor.fetchone()
            return result[0] if result else None
    except Exception as e:
        print(f"Error al obtener token: {e}")
        return None
    finally:
        conn.close()

def token_required(f):
    """Decorador para validar el token en los endpoints"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        token = None
        
        # Obtener el token del header 'Authorization'
        if 'Authorization' in request.headers:
            token = request.headers['Authorization'].split(" ")[1]  # Formato: Bearer <token>
        
        if not token:
            return jsonify({'message': 'Token is missing!'}), 401
        
        # Aquí deberías validar el token. En este ejemplo, verificamos si existe en la BD
        # En un caso real, podrías usar JWT o similar
        conn = get_db_connection()
        if not conn:
            return jsonify({'message': 'Database connection error'}), 500
            
        try:
            with conn.cursor() as cursor:
                cursor.execute("SELECT user_id FROM user_tokens WHERE token = %s", (token,))
                result = cursor.fetchone()
                
                if not result:
                    return jsonify({'message': 'Token is invalid!'}), 401
                
                # Token válido, continuar con la función
                return f(*args, **kwargs)
        except Exception as e:
            print(f"Error al validar token: {e}")
            return jsonify({'message': 'Token validation failed'}), 500
        finally:
            conn.close()
    
    return decorated_function

def allowed_file(filename):
    """Verifica si la extensión del archivo está permitida"""
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in app.config['ALLOWED_EXTENSIONS']

@app.route('/upload', methods=['POST'])
@token_required
def upload_file():
    """Endpoint para subir archivos con autenticación por token"""
    # Verificar si se incluyó un archivo en la petición
    if 'file' not in request.files:
        return jsonify({'message': 'No file part'}), 400
    
    file = request.files['file']
    
    # Si el usuario no selecciona un archivo
    if file.filename == '':
        return jsonify({'message': 'No selected file'}), 400
    
    if file and allowed_file(file.filename):
        # Asegurar el nombre del archivo y generar uno único
        filename = secure_filename(file.filename)
        unique_filename = f"{uuid.uuid4().hex}_{filename}"
        
        # Crear directorio de uploads si no existe
        if not os.path.exists(app.config['UPLOAD_FOLDER']):
            os.makedirs(app.config['UPLOAD_FOLDER'])
        
        # Guardar el archivo
        file.save(os.path.join(app.config['UPLOAD_FOLDER'], unique_filename))
        
        return jsonify({
            'message': 'File uploaded successfully',
            'filename': unique_filename
        }), 200
    else:
        return jsonify({'message': 'File type not allowed'}), 400

if __name__ == '__main__':
    # Crear directorio de uploads si no existe al iniciar la app
    if not os.path.exists(app.config['UPLOAD_FOLDER']):
        os.makedirs(app.config['UPLOAD_FOLDER'])
    
    app.run(debug=True, ssl_context='adhoc')  # Usar SSL en producción real