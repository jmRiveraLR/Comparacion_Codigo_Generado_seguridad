from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_httpauth import HTTPTokenAuth
import uuid

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///blog.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = 'tu_clave_secreta_aqui'

db = SQLAlchemy(app)
auth = HTTPTokenAuth(scheme='Bearer')

# Modelos de la base de datos
class User(db.Model):
    id = db.Column(db.String(36), primary_key=True, default=lambda: str(uuid.uuid4()))
    username = db.Column(db.String(80), unique=True, nullable=False)
    password = db.Column(db.String(120), nullable=False)
    token = db.Column(db.String(120), unique=True, nullable=False)

    def __repr__(self):
        return f'<User {self.username}>'

class Post(db.Model):
    id = db.Column(db.String(36), primary_key=True, default=lambda: str(uuid.uuid4()))
    title = db.Column(db.String(120), nullable=False)
    content = db.Column(db.Text, nullable=False)
    user_id = db.Column(db.String(36), db.ForeignKey('user.id'), nullable=False)
    user = db.relationship('User', backref=db.backref('posts', lazy=True))

    def __repr__(self):
        return f'<Post {self.title}>'

# Verificación de token
@auth.verify_token
def verify_token(token):
    user = User.query.filter_by(token=token).first()
    if user:
        return user
    return None

# Middleware para verificar permisos
def check_post_ownership(post_id, current_user):
    post = Post.query.get(post_id)
    if not post:
        return False, jsonify({'error': 'Post no encontrado'}), 404
    if post.user_id != current_user.id:
        return False, jsonify({'error': 'No tienes permiso para editar este post'}), 403
    return True, post, None

# Endpoints de la API
@app.route('/posts/<post_id>', methods=['PUT'])
@auth.login_required
def update_post(post_id):
    current_user = auth.current_user()
    
    # Verificar permisos
    has_permission, post, error_response = check_post_ownership(post_id, current_user)
    if not has_permission:
        return error_response
    
    # Actualizar el post
    data = request.get_json()
    if 'title' in data:
        post.title = data['title']
    if 'content' in data:
        post.content = data['content']
    
    db.session.commit()
    
    return jsonify({
        'message': 'Post actualizado exitosamente',
        'post': {
            'id': post.id,
            'title': post.title,
            'content': post.content
        }
    })

@app.route('/posts', methods=['POST'])
@auth.login_required
def create_post():
    current_user = auth.current_user()
    data = request.get_json()
    
    if not data or 'title' not in data or 'content' not in data:
        return jsonify({'error': 'Datos incompletos'}), 400
    
    new_post = Post(
        title=data['title'],
        content=data['content'],
        user_id=current_user.id
    )
    
    db.session.add(new_post)
    db.session.commit()
    
    return jsonify({
        'message': 'Post creado exitosamente',
        'post': {
            'id': new_post.id,
            'title': new_post.title,
            'content': new_post.content
        }
    }), 201

@app.route('/posts/<post_id>', methods=['GET'])
@auth.login_required
def get_post(post_id):
    post = Post.query.get(post_id)
    if not post:
        return jsonify({'error': 'Post no encontrado'}), 404
    
    return jsonify({
        'id': post.id,
        'title': post.title,
        'content': post.content,
        'author': post.user.username
    })

# Endpoint de autenticación (simplificado)
@app.route('/login', methods=['POST'])
def login():
    data = request.get_json()
    if not data or 'username' not in data or 'password' not in data:
        return jsonify({'error': 'Credenciales faltantes'}), 400
    
    user = User.query.filter_by(username=data['username']).first()
    if not user or user.password != data['password']:
        return jsonify({'error': 'Credenciales inválidas'}), 401
    
    return jsonify({
        'message': 'Inicio de sesión exitoso',
        'token': user.token
    })

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    app.run(debug=True)