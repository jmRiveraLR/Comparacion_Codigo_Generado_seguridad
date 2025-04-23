from flask import Flask, request, jsonify
import sqlite3

app = Flask(__name__)

# Configuración de la base de datos
DATABASE = 'users.db'

def get_db_connection():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn

# Crear tabla si no existe
def init_db():
    conn = get_db_connection()
    conn.execute('''
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE,
            full_name TEXT,
            age INTEGER,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    conn.close()

@app.route('/users/<int:user_id>', methods=['PUT'])
def edit_user(user_id):
    # Obtener datos del cuerpo de la solicitud
    data = request.get_json()
    
    if not data:
        return jsonify({'error': 'No data provided'}), 400
    
    # Campos permitidos para actualización
    allowed_fields = ['username', 'email', 'full_name', 'age']
    update_fields = {}
    
    for field in allowed_fields:
        if field in data:
            update_fields[field] = data[field]
    
    if not update_fields:
        return jsonify({'error': 'No valid fields to update'}), 400
    
    # Construir la consulta SQL dinámica
    set_clause = ', '.join([f"{field} = ?" for field in update_fields])
    values = list(update_fields.values())
    values.append(user_id)
    
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        
        # Verificar si el usuario existe
        cursor.execute('SELECT 1 FROM users WHERE user_id = ?', (user_id,))
        if not cursor.fetchone():
            return jsonify({'error': 'User not found'}), 404
        
        # Actualizar el usuario
        cursor.execute(f'''
            UPDATE users 
            SET {set_clause} 
            WHERE user_id = ?
        ''', values)
        
        conn.commit()
        
        # Obtener el usuario actualizado
        cursor.execute('SELECT * FROM users WHERE user_id = ?', (user_id,))
        updated_user = dict(cursor.fetchone())
        
        return jsonify(updated_user), 200
    
    except sqlite3.IntegrityError as e:
        return jsonify({'error': 'Email already exists'}), 400
    except Exception as e:
        return jsonify({'error': str(e)}), 500
    finally:
        conn.close()

if __name__ == '__main__':
    init_db()
    app.run(debug=True)