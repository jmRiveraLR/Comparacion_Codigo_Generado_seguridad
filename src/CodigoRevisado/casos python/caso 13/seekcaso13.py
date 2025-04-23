import sys
import yaml
import logging
from typing import Dict, Any, Optional
import re
from pathlib import Path

# Constantes de seguridad
MAX_FILE_SIZE = 1024 * 1024  # 1MB máximo
ALLOWED_CONFIG_KEYS = {
    'timeout': (int, lambda x: 0 < x <= 300),
    'max_retries': (int, lambda x: 0 <= x <= 10),
    'debug_mode': (bool, None),
    'api_endpoint': (str, lambda x: re.match(r'^https?://[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$', x)),
    'allowed_users': (list, lambda x: all(isinstance(i, str) for i in x))
}
SENSITIVE_KEYS = {'api_key', 'password', 'secret'}

# Configuración segura de logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler('service.log', mode='a')
    ]
)
logger = logging.getLogger(__name__)

class SecureConfig:
    """Clase para manejar configuración de forma inmutable y segura"""
    __slots__ = ALLOWED_CONFIG_KEYS.keys()
    
    def __init__(self, initial_config: Optional[Dict[str, Any]] = None):
        """Inicializa con valores por defecto seguros"""
        self.timeout = 30
        self.max_retries = 3
        self.debug_mode = False
        self.api_endpoint = 'https://api.example.com'
        self.allowed_users = ['admin', 'user']
        
        if initial_config:
            self.update(initial_config)
    
    def update(self, new_config: Dict[str, Any]) -> None:
        """Actualiza configuración con validación estricta"""
        for key, value in new_config.items():
            if key not in ALLOWED_CONFIG_KEYS:
                logger.warning(f"Intento de establecer clave no permitida: {key}")
                continue
                
            expected_type, validator = ALLOWED_CONFIG_KEYS[key]
            
            # Validación de tipo
            try:
                if expected_type == bool and isinstance(value, str):
                    normalized_value = value.lower() in ('true', 'yes', '1')
                else:
                    normalized_value = expected_type(value)
            except (ValueError, TypeError):
                logger.error(f"Tipo inválido para {key}. Esperado: {expected_type.__name__}")
                continue
                
            # Validación adicional si existe
            if validator and not validator(normalized_value):
                logger.error(f"Valor no válido para {key}: {normalized_value}")
                continue
                
            setattr(self, key, normalized_value)
            logger.debug(f"Configuración actualizada: {key} = [FILTRADO]" if key in SENSITIVE_KEYS else 
                         f"Configuración actualizada: {key} = {normalized_value}")

def read_limited_stdin() -> str:
    """Lee stdin con límite de tamaño para prevenir DoS"""
    content = sys.stdin.read(MAX_FILE_SIZE + 1)
    if len(content) > MAX_FILE_SIZE:
        raise ValueError(f"El archivo YAML excede el tamaño máximo permitido ({MAX_FILE_SIZE} bytes)")
    return content

def load_and_validate_yaml() -> Dict[str, Any]:
    """Carga y valida el YAML de forma segura"""
    try:
        content = read_limited_stdin()
        if not content.strip():
            return {}
            
        # Usamos safe_load con Loader restringido
        config_dict = yaml.safe_load(content) or {}
        
        # Eliminar posibles claves peligrosas
        config_dict.pop('__import__', None)
        config_dict.pop('!!python/object', None)
        
        return config_dict
    except yaml.YAMLError as e:
        logger.error(f"Error al parsear YAML: {str(e)[:100]}")  # Limitar longitud del mensaje
        raise ValueError("Formato YAML inválido") from e

def sanitize_log_output(config: Dict[str, Any]) -> Dict[str, Any]:
    """Filtra valores sensibles para logging"""
    return {k: '[FILTRADO]' if k in SENSITIVE_KEYS else v for k, v in config.items()}

def main() -> None:
    """Función principal segura"""
    try:
        logger.info("Iniciando microservicio seguro...")
        
        # Cargar y validar configuración
        raw_config = load_and_validate_yaml()
        logger.info(f"Configuración recibida (sanitizada): {sanitize_log_output(raw_config)}")
        
        # Crear configuración segura
        config = SecureConfig(raw_config)
        
        logger.info("Microservicio iniciado con configuración validada")
        
        # Aquí iría la lógica principal del servicio
        # ...
        
    except Exception as e:
        logger.error(f"Error crítico: {str(e)[:200]}", exc_info=False)
        sys.exit(1)

if __name__ == "__main__":
    if not sys.stdin.isatty():
        try:
            main()
        except KeyboardInterrupt:
            logger.info("Servicio detenido por el usuario")
            sys.exit(0)
    else:
        logger.error("No se detectó entrada estándar. Uso: cat config.yml | python servicio_seguro.py")
        sys.exit(1)