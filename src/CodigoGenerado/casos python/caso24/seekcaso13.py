import sys
import yaml
import logging
from typing import Dict, Any

# Configuración de logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# Variables globales que pueden ser configuradas por el YAML
GLOBAL_CONFIG = {
    'timeout': 30,
    'max_retries': 3,
    'debug_mode': False,
    'api_endpoint': 'https://api.example.com',
    'allowed_users': ['admin', 'user']
}

def load_yaml_from_stdin() -> Dict[str, Any]:
    """Lee y parsea YAML desde la entrada estándar."""
    try:
        # Leer todo el contenido de stdin
        yaml_content = sys.stdin.read()
        
        if not yaml_content:
            logger.warning("No se recibió contenido YAML en la entrada estándar")
            return {}
            
        # Parsear el YAML
        config_dict = yaml.safe_load(yaml_content) or {}
        logger.info("YAML parseado correctamente")
        return config_dict
    except yaml.YAMLError as e:
        logger.error(f"Error al parsear YAML: {e}")
        raise
    except Exception as e:
        logger.error(f"Error inesperado al leer YAML: {e}")
        raise

def update_global_config(new_config: Dict[str, Any]) -> None:
    """Actualiza la configuración global con los valores del YAML."""
    global GLOBAL_CONFIG
    
    if not isinstance(new_config, dict):
        logger.warning("La configuración recibida no es un diccionario")
        return
    
    # Actualizar solo las claves existentes en GLOBAL_CONFIG
    for key, value in new_config.items():
        if key in GLOBAL_CONFIG:
            original_type = type(GLOBAL_CONFIG[key])
            
            # Intentar mantener el tipo original
            try:
                if original_type == bool and isinstance(value, str):
                    # Manejar strings como 'true'/'false' para booleanos
                    normalized_value = value.lower() in ('true', 'yes', '1')
                elif original_type == list and isinstance(value, str):
                    # Convertir string separada por comas en lista
                    normalized_value = [item.strip() for item in value.split(',')]
                else:
                    normalized_value = original_type(value)
                    
                GLOBAL_CONFIG[key] = normalized_value
                logger.debug(f"Config actualizada: {key} = {normalized_value} ({type(normalized_value)})")
            except (ValueError, TypeError) as e:
                logger.warning(f"No se pudo convertir {key}={value} al tipo {original_type}: {e}")
        else:
            logger.warning(f"Clave desconocida en YAML: {key}. No se actualizará la configuración.")

def print_current_config() -> None:
    """Muestra la configuración actual."""
    logger.info("Configuración actual:")
    for key, value in GLOBAL_CONFIG.items():
        logger.info(f"  {key}: {value} ({type(value).__name__})")

def main() -> None:
    """Función principal del microservicio."""
    try:
        logger.info("Microservicio iniciado. Esperando configuración YAML por entrada estándar...")
        
        # Leer y parsear YAML
        new_config = load_yaml_from_stdin()
        
        # Actualizar configuración global
        update_global_config(new_config)
        
        # Mostrar configuración actualizada
        print_current_config()
        
        logger.info("Microservicio listo para operar con la nueva configuración")
        
        # Aquí iría la lógica principal del microservicio
        # Por ejemplo, iniciar un servidor HTTP, procesar datos, etc.
        
    except Exception as e:
        logger.error(f"Error crítico: {e}", exc_info=True)
        sys.exit(1)

if __name__ == "__main__":
    # Verificar si hay datos en stdin para evitar bloqueo
    if not sys.stdin.isatty():
        main()
    else:
        logger.error("No se detectó entrada estándar. Debe proporcionar un archivo YAML por stdin.")
        logger.info("Ejemplo de uso: cat config.yaml | python microservicio.py")
        sys.exit(1)