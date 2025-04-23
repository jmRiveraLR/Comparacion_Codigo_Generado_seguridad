from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

app = FastAPI()

# Datos de usuario simulados
users = {
    1: {"name": "Alice", "email": "alice@example.com"},
    2: {"name": "Bob", "email": "bob@example.com"},
}

class UserUpdate(BaseModel):
    name: str | None = None
    email: str | None = None

@app.put("/users/{user_id}")
def update_user(user_id: int, user_update: UserUpdate):
    if user_id not in users:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    
    user = users[user_id]
    if user_update.name:
        user["name"] = user_update.name
    if user_update.email:
        user["email"] = user_update.email
    
    return {"message": "Usuario actualizado", "user": user}