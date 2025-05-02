import sys
import yaml

# Variables globales (ejemplo)
config = {
    "database_url": "",
    "max_connections": 0,
    "debug_mode": False
}

def validate_config(new_config):
    """Valida que el contenido del archivo YAML sea seguro y esperado."""
    if not isinstance(new_config, dict):
        raise ValueError("El archivo YAML debe contener un diccionario en la raíz.")
    
    expected_keys = {"database_url", "max_connections", "debug_mode"}
    for key in new_config:
        if key not in expected_keys:
            raise ValueError(f"Clave inesperada en la configuración: {key}")
    
    if "max_connections" in new_config and not isinstance(new_config["max_connections"], int):
        raise ValueError("El valor de 'max_connections' debe ser un entero.")
    
    if "debug_mode" in new_config and not isinstance(new_config["debug_mode"], bool):
        raise ValueError("El valor de 'debug_mode' debe ser un booleano.")
    
    if "database_url" in new_config and not isinstance(new_config["database_url"], str):
        raise ValueError("El valor de 'database_url' debe ser una cadena de texto.")

def update_globals(new_config):
    """Actualiza las variables globales según el contenido validado del archivo YAML."""
    global config
    config.update(new_config)

def main():
    """Lee un archivo YAML desde la entrada estándar y actualiza las variables globales de forma segura."""
    try:
        # Leer el archivo YAML desde la entrada estándar
        input_data = sys.stdin.read()
        
        # Usar yaml.safe_load() para evitar ejecuciones inseguras
        new_config = yaml.safe_load(input_data)
        
        # Validar el contenido del archivo YAML
        validate_config(new_config)
        
        # Actualizar las variables globales
        update_globals(new_config)
        
        print("Configuración actualizada exitosamente:")
        print(config)
    except Exception as e:
        print(f"Error procesando el archivo YAML: {e}")

if __name__ == "__main__":
    main()