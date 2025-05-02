from flask import Flask, request, jsonify
import sqlite3
import re
from functools import wraps
from werkzeug.security import generate_password_hash

app = Flask(__name__)
app.config['SECRET_KEY'] = 'your-secret-key-here'  # Debería ser una variable de entorno en producción

# Configuración de seguridad
app.config.update(
    SESSION_COOKIE_SECURE=True,
    SESSION_COOKIE_HTTPONLY=True,
    SESSION_COOKIE_SAMESITE='Lax',
)

# Configuración de la base de datos
DATABASE = 'users.db'

# Decorador para autenticación y autorización
def token_required(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        token = request.headers.get('Authorization')
        if not token:
            return jsonify({'error': 'Token is missing'}), 401
        # Aquí deberías validar el token contra tu sistema de autenticación
        # Ejemplo simplificado:
        if token != "Bearer valid-token":
            return jsonify({'error': 'Invalid token'}), 403
        return f(*args, **kwargs)
    return decorated

def get_db_connection():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn

# Validación de campos de entrada
def validate_input(data):
    if 'email' in data and not re.match(r"[^@]+@[^@]+\.[^@]+", data['email']):
        return False
    if 'username' in data and not re.match(r"^[a-zA-Z0-9_]{3,20}$", data['username']):
        return False
    if 'age' in data and (not isinstance(data['age'], int) or data['age'] < 0 or data['age'] > 120):
        return False
    return True

# Crear tabla si no existe
def init_db():
    conn = get_db_connection()
    conn.execute('''
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            email TEXT NOT NULL UNIQUE,
            full_name TEXT,
            age INTEGER,
            password_hash TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    conn.close()

@app.route('/users/<int:user_id>', methods=['PUT'])
@token_required
def edit_user(user_id):
    # Validar Content-Type
    if request.content_type != 'application/json':
        return jsonify({'error': 'Unsupported Media Type'}), 415
    
    # Obtener datos del cuerpo de la solicitud
    data = request.get_json()
    
    if not data:
        return jsonify({'error': 'No data provided'}), 400
    
    # Validar campos de entrada
    if not validate_input(data):
        return jsonify({'error': 'Invalid input data'}), 400
    
    # Campos permitidos para actualización (lista blanca)
    allowed_fields = {'username', 'email', 'full_name', 'age'}
    update_fields = {k: v for k, v in data.items() if k in allowed_fields}
    
    if not update_fields:
        return jsonify({'error': 'No valid fields to update'}), 400
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Verificar si el usuario existe
        cursor.execute('SELECT 1 FROM users WHERE user_id = ?', (user_id,))
        if not cursor.fetchone():
            return jsonify({'error': 'User not found'}), 404
        
        # Construir consulta SQL de forma segura
        set_clause = ', '.join([f"{field} = ?" for field in update_fields])
        values = list(update_fields.values())
        values.append(user_id)
        
        # Actualizar el usuario
        cursor.execute(f'''
            UPDATE users 
            SET {set_clause} 
            WHERE user_id = ?
        ''', values)
        
        conn.commit()
        
        # Obtener el usuario actualizado
        cursor.execute('SELECT user_id, username, email, full_name, age FROM users WHERE user_id = ?', (user_id,))
        user = cursor.fetchone()
        
        if not user:
            return jsonify({'error': 'User not found after update'}), 404
            
        return jsonify(dict(user)), 200
    
    except sqlite3.IntegrityError as e:
        return jsonify({'error': 'Database integrity error (possibly duplicate username or email)'}), 400
    except Exception as e:
        app.logger.error(f"Error updating user: {str(e)}")
        return jsonify({'error': 'Internal server error'}), 500
    finally:
        conn.close()

if __name__ == '__main__':
    init_db()
    # En producción, debug debe ser False
    app.run(debug=False, host='0.0.0.0', port=5000)