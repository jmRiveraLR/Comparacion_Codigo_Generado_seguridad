from flask import Flask, request, jsonify, abort
from flask_wtf.csrf import CSRFProtect
from flask_login import current_user, LoginManager, login_required, UserMixin

app = Flask(__name__)
app.secret_key = 'super-secret-key'  # Debe usarse una clave segura desde variables de entorno en producción
csrf = CSRFProtect(app)
login_manager = LoginManager(app)

# Simulación de base de datos y usuarios autenticados
users = {
    1: {"username": "alice", "email": "alice@example.com"},
    2: {"username": "bob", "email": "bob@example.com"},
    3: {"username": "charlie", "email": "charlie@example.com"}
}

# Simulación de usuarios para login manager
class User(UserMixin):
    def __init__(self, id):
        self.id = id

@login_manager.user_loader
def load_user(user_id):
    if int(user_id) in users:
        return User(user_id)
    return None

@app.route('/users/<int:user_id>', methods=['PUT'])
@csrf.exempt  # Quitar esto si se requiere protección CSRF en clientes no REST
@login_required
def update_user(user_id):
    if int(current_user.id) != user_id:
        abort(403)  # No autorizado

    data = request.get_json()
    if not data:
        abort(400)

    username = data.get('username')
    email = data.get('email')

    if username:
        users[user_id]['username'] = username
    if email:
        users[user_id]['email'] = email

    return jsonify({"message": "Usuario actualizado", "user": users[user_id]}), 200

if __name__ == '__main__':
    # Nunca activar debug en producción
    app.run(debug=False)
