import sys
import yaml
from fastapi import FastAPI
from pydantic import BaseModel
from typing import Optional
import uvicorn

# Variables globales que se actualizarán
CONFIG = {
    "debug": False,
    "timeout": 30,
    "service_name": "DefaultService"
}

app = FastAPI()

class ConfigUpdate(BaseModel):
    debug: Optional[bool]
    timeout: Optional[int]
    service_name: Optional[str]

def update_globals_from_dict(config_dict):
    for key, value in config_dict.items():
        if key in CONFIG:
            CONFIG[key] = value

@app.get("/config")
def get_config():
    return CONFIG

@app.post("/update-config")
def update_config(new_config: ConfigUpdate):
    update_globals_from_dict(new_config.dict(exclude_unset=True))
    return {"status": "updated", "config": CONFIG}

if __name__ == "__main__":
    # Leer el YAML desde la entrada estándar
    print("Esperando archivo YAML desde entrada estándar... (Ctrl+D para finalizar)")
    try:
        input_data = sys.stdin.read()
        parsed_yaml = yaml.safe_load(input_data)
        if isinstance(parsed_yaml, dict):
            update_globals_from_dict(parsed_yaml)
            print("Configuración inicial cargada:", CONFIG)
        else:
            print("El archivo YAML debe contener un diccionario.")
    except Exception as e:
        print("Error al procesar el archivo YAML:", e)

    # Iniciar el microservicio
    uvicorn.run(app, host="0.0.0.0", port=8000)
