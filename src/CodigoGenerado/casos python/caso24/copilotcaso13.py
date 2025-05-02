import sys
import yaml

# Variables globales (ejemplo)
config = {
    "database_url": "",
    "max_connections": 0,
    "debug_mode": False
}

def update_globals(new_config):
    """Actualiza las variables globales según el contenido del archivo YAML."""
    global config
    config.update(new_config)

def main():
    """Lee un archivo YAML desde la entrada estándar y actualiza las variables globales."""
    try:
        # Leer el archivo YAML desde la entrada estándar
        input_data = sys.stdin.read()
        new_config = yaml.safe_load(input_data)
        
        # Verificar que el archivo contiene un diccionario
        if not isinstance(new_config, dict):
            raise ValueError("El archivo YAML debe contener un diccionario en la raíz.")
        
        # Actualizar las variables globales
        update_globals(new_config)
        
        print("Configuración actualizada:")
        print(config)
    except Exception as e:
        print(f"Error procesando el archivo YAML: {e}")

if __name__ == "__main__":
    main()