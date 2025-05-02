from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from flask_jwt_extended import JWTManager, create_access_token, jwt_required, get_jwt_identity

app = Flask(__name__)
app.config["SQLALCHEMY_DATABASE_URI"] = "sqlite:///database.db"
app.config["JWT_SECRET_KEY"] = "supersecretkey"
db = SQLAlchemy(app)
jwt = JWTManager(app)

class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password = db.Column(db.String(80), nullable=False)

class Post(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey("user.id"), nullable=False)
    content = db.Column(db.Text, nullable=False)

@app.route("/login", methods=["POST"])
def login():
    username = request.json.get("username")
    password = request.json.get("password")
    user = User.query.filter_by(username=username, password=password).first()
    if user:
        access_token = create_access_token(identity=user.id)
        return jsonify(access_token=access_token), 200
    return jsonify({"msg": "Invalid credentials"}), 401

@app.route("/post/<int:post_id>", methods=["PUT"])
@jwt_required()
def edit_post(post_id):
    user_id = get_jwt_identity()
    post = Post.query.filter_by(id=post_id, user_id=user_id).first()

    if post:
        post.content = request.json.get("content")
        db.session.commit()
        return jsonify({"msg": "Post updated successfully"}), 200
    return jsonify({"msg": "Unauthorized"}), 403

if __name__ == "__main__":
    with app.app_context():
        db.create_all()
    app.run(debug=True)