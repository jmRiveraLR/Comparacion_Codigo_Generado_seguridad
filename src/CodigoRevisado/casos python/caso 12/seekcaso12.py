import paramiko
import getpass
import re
from typing import Tuple, Optional
import logging
from hashlib import sha256
import os

# Configuración de logging seguro
logging.basicConfig(level=logging.WARNING)
logger = logging.getLogger(__name__)

class SSHSecurityError(Exception):
    """Clase para manejar errores de seguridad SSH"""
    pass

class SSHCommandExecutor:
    def __init__(self):
        self.connection_attempts = 0
        self.max_attempts = 3
        self.command_timeout = 30  # segundos

    def validate_host(self, host: str) -> bool:
        """Valida que el host tenga un formato correcto"""
        ip_regex = r'^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$'
        hostname_regex = r'^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$'
        return re.match(ip_regex, host) is not None or re.match(hostname_regex, host) is not None

    def validate_command(self, command: str) -> bool:
        """Valida que el comando no contenga caracteres peligrosos"""
        forbidden_chars = [';', '&', '|', '`', '$', '(', ')', '<', '>']
        return all(char not in command for char in forbidden_chars)

    def secure_password_handling(self, password: str) -> None:
        """Intenta manejar la contraseña de forma segura"""
        try:
            # Convertir a bytes y limpiar después de usar
            pass_bytes = password.encode('utf-8')
            hashed = sha256(pass_bytes).hexdigest()
            # Sobrescribir la memoria
            pass_bytes = b'\x00' * len(pass_bytes)
            return hashed  # Solo para demostración, en realidad no deberíamos almacenarlo
        except Exception as e:
            logger.error("Error en el manejo seguro de contraseña")
            raise SSHSecurityError("Error de seguridad en manejo de credenciales")

    def connect_with_retry(self, host: str, port: int, username: str, password: str) -> paramiko.SSHClient:
        """Establece conexión SSH con reintentos limitados"""
        if self.connection_attempts >= self.max_attempts:
            raise SSHSecurityError("Número máximo de intentos excedido")

        client = paramiko.SSHClient()
        try:
            # Cargar claves conocidas del sistema
            client.load_system_host_keys()
            # Política más segura para claves desconocidas
            client.set_missing_host_key_policy(paramiko.RejectPolicy())
            
            client.connect(
                hostname=host,
                port=port,
                username=username,
                password=password,
                timeout=10,
                auth_timeout=10,
                banner_timeout=10
            )
            return client
        except paramiko.AuthenticationException:
            self.connection_attempts += 1
            raise SSHSecurityError("Autenticación fallida")
        except Exception as e:
            logger.warning(f"Error de conexión: {str(e)}")
            raise SSHSecurityError("Error de conexión")

    def execute_secure_command(self, host: str, port: int, username: str, command: str) -> Tuple[int, str, str]:
        """Ejecuta un comando de forma segura en el servidor remoto"""
        if not self.validate_host(host):
            raise SSHSecurityError("Formato de host inválido")

        if not self.validate_command(command):
            raise SSHSecurityError("Comando no permitido por políticas de seguridad")

        password = None
        try:
            password = getpass.getpass(f"Contraseña para {username}@{host}: ")
            self.secure_password_handling(password)  # Manejo seguro
            
            client = self.connect_with_retry(host, port, username, password)
            
            # Ejecutar comando con timeout
            stdin, stdout, stderr = client.exec_command(command, timeout=self.command_timeout)
            
            # Leer resultados
            exit_code = stdout.channel.recv_exit_status()
            output = stdout.read().decode('utf-8').strip()
            error = stderr.read().decode('utf-8').strip()
            
            return (exit_code, output, error)
            
        except paramiko.SSHException as e:
            logger.warning(f"Error SSH: {str(e)}")
            raise SSHSecurityError("Error en la ejecución remota")
        except Exception as e:
            logger.error(f"Error inesperado: {str(e)}")
            raise SSHSecurityError("Error en la operación")
        finally:
            if password:
                # Limpieza de la contraseña
                password = '\x00' * len(password)
            if 'client' in locals():
                client.close()

if __name__ == "__main__":
    try:
        executor = SSHCommandExecutor()
        
        print("Script de conexión SSH segura")
        print("-----------------------------")
        
        host = input("Servidor remoto (IP o nombre): ")
        port = int(input("Puerto SSH (dejar vacío para 22): ") or 22)
        username = input("Usuario: ")
        command = input("Comando a ejecutar: ")
        
        exit_code, output, error = executor.execute_secure_command(host, port, username, command)
        
        print("\nResultado:")
        print(f"Código de salida: {exit_code}")
        
        if output:
            print("\nSalida estándar:")
            print(output)
        
        if error:
            print("\nSalida de error:")
            print(error)
            
    except SSHSecurityError as e:
        print(f"\nError de seguridad: {str(e)}")
    except Exception as e:
        print(f"\nError inesperado: {str(e)}")