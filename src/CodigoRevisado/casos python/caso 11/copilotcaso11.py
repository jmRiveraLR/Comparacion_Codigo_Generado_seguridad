from fastapi import FastAPI, HTTPException, Depends
from pydantic import BaseModel, EmailStr
from typing import Dict
from fastapi.security import OAuth2PasswordBearer, OAuth2PasswordRequestForm

app = FastAPI()

# Configuración de autenticación
oauth2_scheme = OAuth2PasswordBearer(tokenUrl="token")

# Datos de usuario simulados
users = {
    1: {"name": "Alice", "email": "alice@example.com", "owner": "user1"},
    2: {"name": "Bob", "email": "bob@example.com", "owner": "user2"},
}

# Base de datos simulada de tokens y roles
tokens = {"valid_token_user1": "user1", "valid_token_user2": "user2"}

class UserUpdate(BaseModel):
    name: str | None = None
    email: EmailStr | None = None  # Validación estricta de correo electrónico

# Función para verificar autenticación
def get_current_user(token: str = Depends(oauth2_scheme)) -> str:
    user = tokens.get(token)
    if not user:
        raise HTTPException(status_code=401, detail="Token inválido o usuario no autenticado")
    return user

@app.put("/users/{user_id}")
def update_user(user_id: int, user_update: UserUpdate, current_user: str = Depends(get_current_user)):
    user = users.get(user_id)
    if not user:
        raise HTTPException(status_code=404, detail="Usuario no encontrado")
    
    # Verificación de control de acceso
    if user["owner"] != current_user:
        raise HTTPException(status_code=403, detail="No tienes permiso para editar este usuario")
    
    # Actualización con validación
    if user_update.name:
        user["name"] = user_update.name
    if user_update.email:
        user["email"] = user_update.email
    
    return {"message": "Usuario actualizado", "user": user}