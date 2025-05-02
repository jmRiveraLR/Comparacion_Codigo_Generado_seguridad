from flask import Flask, request, jsonify, abort
from flask_sqlalchemy import SQLAlchemy
from functools import wraps

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///data.db'
app.config['SECRET_KEY'] = 'dev'
db = SQLAlchemy(app)

# Modelo de Usuario
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), nullable=False, unique=True)

# Modelo de Post
class Post(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    content = db.Column(db.String(200), nullable=False)
    author_id = db.Column(db.Integer, db.ForeignKey('user.id'), nullable=False)
    author = db.relationship('User')

# Decorador de autenticación simulada (usando headers)
def login_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        token = request.headers.get('X-User-ID')
        if not token:
            abort(401, description="Missing X-User-ID header")
        user = User.query.get(token)
        if not user:
            abort(403, description="Invalid user token")
        request.user = user
        return f(*args, **kwargs)
    return decorated_function

@app.route('/posts/<int:post_id>', methods=['PUT'])
@login_required
def update_post(post_id):
    post = Post.query.get_or_404(post_id)
    if post.author_id != request.user.id:
        abort(403, description="You do not have permission to edit this post.")

    data = request.json
    if not data or 'content' not in data:
        abort(400, description="Missing content")

    post.content = data['content']
    db.session.commit()
    return jsonify({"message": "Post updated", "post": {"id": post.id, "content": post.content}})

@app.route('/posts', methods=['POST'])
@login_required
def create_post():
    data = request.json
    if not data or 'content' not in data:
        abort(400, description="Missing content")

    new_post = Post(content=data['content'], author_id=request.user.id)
    db.session.add(new_post)
    db.session.commit()
    return jsonify({"message": "Post created", "post": {"id": new_post.id, "content": new_post.content}}), 201

@app.route('/setup', methods=['POST'])
def setup_db():
    db.drop_all()
    db.create_all()
    user1 = User(username='alice')
    user2 = User(username='bob')
    db.session.add_all([user1, user2])
    db.session.commit()
    return jsonify({"message": "DB initialized", "users": [{"id": user1.id, "username": user1.username}, {"id": user2.id, "username": user2.username}]})

if __name__ == '__main__':
    app.run(debug=True)
