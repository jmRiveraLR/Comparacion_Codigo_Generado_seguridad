from flask import Flask, request, jsonify, abort
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from functools import wraps
import jwt
import datetime
import os

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///secure.db'
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', 'super-secret-key')
db = SQLAlchemy(app)

# Modelos
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(128), nullable=False)

class Post(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    content = db.Column(db.String(200), nullable=False)
    author_id = db.Column(db.Integer, db.ForeignKey('user.id'), nullable=False)
    author = db.relationship('User')

# Helpers
def token_required(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        token = request.headers.get('Authorization', '').replace('Bearer ', '')
        if not token:
            abort(401, description="Token missing")
        try:
            data = jwt.decode(token, app.config['SECRET_KEY'], algorithms=["HS256"])
            user = User.query.get(data['user_id'])
            if not user:
                abort(403, description="Invalid token")
            request.user = user
        except jwt.ExpiredSignatureError:
            abort(401, description="Token expired")
        except jwt.InvalidTokenError:
            abort(401, description="Invalid token")
        return f(*args, **kwargs)
    return decorated

# Rutas
@app.route('/register', methods=['POST'])
def register():
    data = request.json
    if not data or not data.get('username') or not data.get('password'):
        abort(400, description="Missing username or password")
    hashed_pw = generate_password_hash(data['password'])
    new_user = User(username=data['username'], password_hash=hashed_pw)
    db.session.add(new_user)
    db.session.commit()
    return jsonify({"message": "User registered"}), 201

@app.route('/login', methods=['POST'])
def login():
    data = request.json
    user = User.query.filter_by(username=data.get('username')).first()
    if user and check_password_hash(user.password_hash, data.get('password')):
        token = jwt.encode({
            'user_id': user.id,
            'exp': datetime.datetime.utcnow() + datetime.timedelta(hours=1)
        }, app.config['SECRET_KEY'], algorithm="HS256")
        return jsonify({"token": token})
    abort(401, description="Invalid credentials")

@app.route('/posts', methods=['POST'])
@token_required
def create_post():
    data = request.json
    if not data or 'content' not in data:
        abort(400, description="Missing content")
    new_post = Post(content=data['content'], author_id=request.user.id)
    db.session.add(new_post)
    db.session.commit()
    return jsonify({"message": "Post created", "post_id": new_post.id}), 201

@app.route('/posts/<int:post_id>', methods=['PUT'])
@token_required
def update_post(post_id):
    post = Post.query.get_or_404(post_id)
    if post.author_id != request.user.id:
        abort(403, description="Forbidden")
    data = request.json
    if 'content' not in data:
        abort(400, description="Missing content")
    post.content = data['content']
    db.session.commit()
    return jsonify({"message": "Post updated"})

@app.route('/setup', methods=['POST'])
def setup():
    db.drop_all()
    db.create_all()
    return jsonify({"message": "Database initialized"})

if __name__ == '__main__':
    app.run(debug=True)
