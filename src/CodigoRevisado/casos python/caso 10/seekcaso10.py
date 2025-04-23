import os
from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_httpauth import HTTPTokenAuth
from werkzeug.security import generate_password_hash, check_password_hash
from flask_wtf.csrf import CSRFProtect
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
import secrets
import logging
from functools import wraps

# Configuración inicial
app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv('DATABASE_URI', 'sqlite:///blog.db')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', secrets.token_hex(32))
app.config['WTF_CSRF_ENABLED'] = True

# Extensiones de seguridad
db = SQLAlchemy(app)
auth = HTTPTokenAuth(scheme='Bearer')
csrf = CSRFProtect(app)
limiter = Limiter(
    app,
    key_func=get_remote_address,
    default_limits=["200 per day", "50 per hour"]
)

# Configuración de logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Modelos de la base de datos
class User(db.Model):
    id = db.Column(db.String(36), primary_key=True, default=lambda: str(secrets.token_hex(18)))
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(256), nullable=False)
    token = db.Column(db.String(128), unique=True, nullable=False)

    def set_password(self, password):
        self.password_hash = generate_password_hash(password)

    def check_password(self, password):
        return check_password_hash(self.password_hash, password)

    def generate_token(self):
        self.token = secrets.token_urlsafe(64)
        return self.token

class Post(db.Model):
    id = db.Column(db.String(36), primary_key=True, default=lambda: str(secrets.token_hex(18)))
    title = db.Column(db.String(120), nullable=False)
    content = db.Column(db.Text, nullable=False)
    user_id = db.Column(db.String(36), db.ForeignKey('user.id'), nullable=False)
    user = db.relationship('User', backref=db.backref('posts', lazy=True))

# Verificación de token
@auth.verify_token
def verify_token(token):
    try:
        user = User.query.filter_by(token=token).first()
        if user:
            logger.info(f"Acceso autorizado para usuario: {user.username}")
            return user
        logger.warning("Intento de acceso con token inválido")
        return None
    except Exception as e:
        logger.error(f"Error en verificación de token: {str(e)}")
        return None

# Decorador para registro de auditoría
def audit_log(action):
    def decorator(f):
        @wraps(f)
        def decorated_function(*args, **kwargs):
            user = auth.current_user() if auth.current_user() else None
            user_id = user.id if user else 'anon'
            logger.info(f"Intento de {action} por usuario {user_id}")
            return f(*args, **kwargs)
        return decorated_function
    return decorator

# Middleware para verificar permisos
def check_post_ownership(post_id, current_user):
    try:
        post = Post.query.get(post_id)
        if not post:
            logger.warning(f"Post {post_id} no encontrado")
            return False, jsonify({'error': 'Post no encontrado'}), 404
        if post.user_id != current_user.id:
            logger.warning(f"Intento de acceso no autorizado a post {post_id} por usuario {current_user.id}")
            return False, jsonify({'error': 'No tienes permiso para editar este post'}), 403
        return True, post, None
    except Exception as e:
        logger.error(f"Error en verificación de permisos: {str(e)}")
        return False, jsonify({'error': 'Error interno del servidor'}), 500

# Endpoints de la API
@app.route('/posts/<post_id>', methods=['PUT'])
@auth.login_required
@csrf.exempt  # Exención controlada para API
@audit_log('actualización de post')
def update_post(post_id):
    current_user = auth.current_user()
    
    has_permission, post, error_response = check_post_ownership(post_id, current_user)
    if not has_permission:
        return error_response
    
    data = request.get_json()
    if not data:
        return jsonify({'error': 'Datos no proporcionados'}), 400
    
    try:
        if 'title' in data:
            post.title = data['title']
        if 'content' in data:
            post.content = data['content']
        
        db.session.commit()
        logger.info(f"Post {post_id} actualizado por usuario {current_user.id}")
        
        return jsonify({
            'message': 'Post actualizado exitosamente',
            'post': {
                'id': post.id,
                'title': post.title,
                'content': post.content
            }
        })
    except Exception as e:
        db.session.rollback()
        logger.error(f"Error al actualizar post {post_id}: {str(e)}")
        return jsonify({'error': 'Error al actualizar el post'}), 500

@app.route('/posts', methods=['POST'])
@auth.login_required
@csrf.exempt  # Exención controlada para API
@limiter.limit("5 per minute")
@audit_log('creación de post')
def create_post():
    current_user = auth.current_user()
    data = request.get_json()
    
    if not data or 'title' not in data or 'content' not in data:
        return jsonify({'error': 'Datos incompletos'}), 400
    
    try:
        new_post = Post(
            title=data['title'],
            content=data['content'],
            user_id=current_user.id
        )
        
        db.session.add(new_post)
        db.session.commit()
        logger.info(f"Nuevo post creado por usuario {current_user.id}")
        
        return jsonify({
            'message': 'Post creado exitosamente',
            'post': {
                'id': new_post.id,
                'title': new_post.title,
                'content': new_post.content
            }
        }), 201
    except Exception as e:
        db.session.rollback()
        logger.error(f"Error al crear post: {str(e)}")
        return jsonify({'error': 'Error al crear el post'}), 500

@app.route('/login', methods=['POST'])
@limiter.limit("5 per minute")
@audit_log('inicio de sesión')
def login():
    data = request.get_json()
    if not data or 'username' not in data or 'password' not in data:
        return jsonify({'error': 'Credenciales faltantes'}), 400
    
    try:
        user = User.query.filter_by(username=data['username']).first()
        if not user or not user.check_password(data['password']):
            logger.warning(f"Intento fallido de inicio de sesión para usuario {data['username']}")
            return jsonify({'error': 'Credenciales inválidas'}), 401
        
        token = user.generate_token()
        db.session.commit()
        
        return jsonify({
            'message': 'Inicio de sesión exitoso',
            'token': token
        })
    except Exception as e:
        logger.error(f"Error en proceso de login: {str(e)}")
        return jsonify({'error': 'Error en el servidor'}), 500

if __name__ == '__main__':
    with app.app_context():
        db.create_all()
    # Solo modo debug en desarrollo
    app.run(debug=os.getenv('FLASK_DEBUG', 'False') == 'True')