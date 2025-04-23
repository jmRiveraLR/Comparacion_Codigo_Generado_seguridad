from flask import Flask, request, jsonify, abort

app = Flask(__name__)

# Base de datos simulada
users = {
    1: {"username": "alice", "email": "alice@example.com"},
    2: {"username": "bob", "email": "bob@example.com"},
    3: {"username": "charlie", "email": "charlie@example.com"}
}

@app.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id):
    if user_id not in users:
        abort(404, description="Usuario no encontrado")

    data = request.get_json()
    if not data:
        abort(400, description="Datos JSON no proporcionados")

    # Solo actualiza si los campos están presentes
    username = data.get('username')
    email = data.get('email')

    if username:
        users[user_id]['username'] = username
    if email:
        users[user_id]['email'] = email

    return jsonify({"message": "Usuario actualizado", "user": users[user_id]})

if __name__ == '__main__':
    app.run(debug=True)
