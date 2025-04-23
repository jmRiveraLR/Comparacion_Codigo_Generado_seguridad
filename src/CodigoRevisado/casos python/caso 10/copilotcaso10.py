from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_jwt_extended import JWTManager, create_access_token, jwt_required, get_jwt_identity
from flask_wtf.csrf import CSRFProtect
from werkzeug.security import generate_password_hash, check_password_hash
import logging
import os

app = Flask(__name__)
app.config["SQLALCHEMY_DATABASE_URI"] = "sqlite:///database.db"
app.config["JWT_SECRET_KEY"] = os.getenv("JWT_SECRET_KEY", "change_this_in_production")  # Mejor gestión de secretos
app.config["WTF_CSRF_ENABLED"] = True  # Activación de protección contra CSRF

db = SQLAlchemy(app)
jwt = JWTManager(app)
csrf = CSRFProtect(app)

# Configuración de logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(message)s")

class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password = db.Column(db.String(200), nullable=False)  # Mayor longitud para hashes seguros

class Post(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey("user.id"), nullable=False)
    content = db.Column(db.Text, nullable=False)

@app.route("/login", methods=["POST"])
def login():
    username = request.json.get("username")
    password = request.json.get("password")
    user = User.query.filter_by(username=username).first()
    if user and check_password_hash(user.password, password):  # Validación de contraseñas con hash seguro
        access_token = create_access_token(identity=user.id)
        return jsonify(access_token=access_token), 200
    return jsonify({"msg": "Invalid credentials"}), 401

@app.route("/post/<int:post_id>", methods=["PUT"])
@jwt_required()
@csrf.exempt  # Excepción para solicitudes autenticadas
def edit_post(post_id):
    user_id = get_jwt_identity()
    post = Post.query.filter_by(id=post_id, user_id=user_id).first()

    if post:
        content = request.json.get("content")
        if not isinstance(content, str) or len(content.strip()) == 0:  # Validación estricta de entrada
            return jsonify({"msg": "Invalid input"}), 400
        post.content = content
        db.session.commit()
        logging.info(f"Post {post_id} edited by user {user_id}")
        return jsonify({"msg": "Post updated successfully"}), 200
    logging.warning(f"Unauthorized edit attempt for post {post_id} by user {user_id}")
    return jsonify({"msg": "Unauthorized"}), 403

if __name__ == "__main__":
    with app.app_context():
        db.create_all()
    app.run(debug=False)  # Modo debug desactivado