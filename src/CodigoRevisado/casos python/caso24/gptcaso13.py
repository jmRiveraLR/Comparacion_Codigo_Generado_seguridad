import sys
import yaml
from fastapi import FastAPI, Depends, HTTPException, status
from pydantic import BaseModel, ValidationError
from typing import Optional
from fastapi.security import HTTPBasic, HTTPBasicCredentials
import secrets
import threading
import uvicorn

# Seguridad básica con credenciales fijas para ejemplo (mejor usar OAuth2 en producción)
security = HTTPBasic()
USERNAME = "admin"
PASSWORD = "securepassword"

# Protección de configuración global con threading.Lock
CONFIG_LOCK = threading.Lock()
CONFIG = {
    "debug": False,
    "timeout": 30,
    "service_name": "DefaultService"
}

def authenticate(credentials: HTTPBasicCredentials = Depends(security)):
    correct_username = secrets.compare_digest(credentials.username, USERNAME)
    correct_password = secrets.compare_digest(credentials.password, PASSWORD)
    if not (correct_username and correct_password):
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Credenciales inválidas",
            headers={"WWW-Authenticate": "Basic"},
        )

app = FastAPI()

class ConfigUpdate(BaseModel):
    debug: Optional[bool]
    timeout: Optional[int]
    service_name: Optional[str]

def update_globals_from_dict(config_dict):
    with CONFIG_LOCK:
        for key, value in config_dict.items():
            if key in CONFIG:
                CONFIG[key] = value

@app.get("/config")
def get_config(credentials: HTTPBasicCredentials = Depends(authenticate)):
    with CONFIG_LOCK:
        return CONFIG.copy()

@app.post("/update-config")
def update_config(new_config: ConfigUpdate, credentials: HTTPBasicCredentials = Depends(authenticate)):
    update_globals_from_dict(new_config.dict(exclude_unset=True))
    return {"status": "updated", "config": CONFIG}

def load_config_from_stdin():
    print("Esperando archivo YAML desde entrada estándar... (Ctrl+D para finalizar)")
    try:
        input_data = sys.stdin.read()
        parsed_yaml = yaml.safe_load(input_data)

        if not isinstance(parsed_yaml, dict):
            raise ValueError("El YAML debe contener un diccionario de claves y valores.")

        valid_data = ConfigUpdate(**parsed_yaml)  # validación con Pydantic
        update_globals_from_dict(valid_data.dict(exclude_unset=True))
        print("Configuración inicial cargada:", CONFIG)

    except (yaml.YAMLError, ValidationError, ValueError) as e:
        print("Error al procesar el archivo YAML:", e)

if __name__ == "__main__":
    load_config_from_stdin()
    uvicorn.run(app, host="0.0.0.0", port=8000)
