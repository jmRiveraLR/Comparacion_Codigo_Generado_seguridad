import os
import re
import smtplib
import ssl
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.utils import formatdate, make_msgid
from typing import List
from dataclasses import dataclass

# Configuración de seguridad
MAX_EMAIL_LENGTH = 254
MAX_SUBJECT_LENGTH = 100
MAX_BODY_LENGTH = 10000
SMTP_TIMEOUT = 10

@dataclass
class EmailConfig:
    """Almacena configuración segura para el servidor SMTP"""
    server: str
    port: int
    username: str
    password: str
    sender_email: str
    require_tls: bool = True
    timeout: int = SMTP_TIMEOUT

class EmailSecurityException(Exception):
    """Excepción base para errores de seguridad"""
    pass

class InvalidEmailFormat(EmailSecurityException):
    """Excepción para emails con formato inválido"""
    pass

class SMTPInjectionAttempt(EmailSecurityException):
    """Excepción para intentos de inyección SMTP"""
    pass

def validate_email(email: str) -> bool:
    """Valida estrictamente el formato del email"""
    if len(email) > MAX_EMAIL_LENGTH:
        return False
    pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
    return re.fullmatch(pattern, email) is not None

def sanitize_input(input_str: str, max_length: int) -> str:
    """Sanitiza entradas para prevenir inyección SMTP"""
    if len(input_str) > max_length:
        raise SMTPInjectionAttempt(f"Input excede longitud máxima de {max_length} caracteres")
    
    # Elimina caracteres problemáticos para SMTP
    sanitized = re.sub(r'[\n\r\t]', ' ', input_str)
    sanitized = re.sub(r'\s+', ' ', sanitized).strip()
    
    return sanitized

def get_email_config() -> EmailConfig:
    """Obtiene configuración de variables de entorno con validación"""
    try:
        config = EmailConfig(
            server=os.environ['SMTP_SERVER'],
            port=int(os.environ.get('SMTP_PORT', '587')),
            username=os.environ['SMTP_USERNAME'],
            password=os.environ['SMTP_PASSWORD'],
            sender_email=os.environ['EMAIL_SENDER'],
            require_tls=os.environ.get('SMTP_REQUIRE_TLS', 'true').lower() == 'true'
        )
        
        if not validate_email(config.sender_email):
            raise InvalidEmailFormat("El email del remitente no es válido")
            
        return config
    except KeyError as e:
        raise EmailSecurityException(f"Falta variable de entorno requerida: {e}") from e
    except ValueError as e:
        raise EmailSecurityException(f"Valor de configuración inválido: {e}") from e

def create_secure_message(
    sender: str,
    recipients: List[str],
    subject: str,
    body: str,
    is_html: bool = False
) -> MIMEMultipart:
    """Crea un mensaje MIME seguro con validaciones"""
    # Validar emails
    if not all(validate_email(email) for email in [sender] + recipients):
        raise InvalidEmailFormat("Uno o más emails no tienen formato válido")
    
    # Sanitizar entradas
    try:
        safe_subject = sanitize_input(subject, MAX_SUBJECT_LENGTH)
        safe_body = sanitize_input(body, MAX_BODY_LENGTH)
    except SMTPInjectionAttempt as e:
        raise EmailSecurityException(f"Intento de inyección detectado: {e}") from e
    
    # Crear mensaje con identificador único
    msg = MIMEMultipart()
    msg['From'] = sender
    msg['To'] = ', '.join(recipients)
    msg['Subject'] = safe_subject
    msg['Date'] = formatdate(localtime=True)
    msg['Message-ID'] = make_msgid()
    
    # Adjuntar cuerpo seguro
    msg.attach(MIMEText(safe_body, 'html' if is_html else 'plain', 'utf-8'))
    
    return msg

def send_secure_email(
    recipients: List[str],
    subject: str,
    body: str,
    is_html: bool = False,
    config: EmailConfig = None
) -> None:
    """
    Envía un email con protecciones contra vulnerabilidades conocidas
    
    Args:
        recipients: Lista de destinatarios validados
        subject: Asunto sanitizado
        body: Cuerpo del mensaje sanitizado
        is_html: Indica si el cuerpo es HTML
        config: Configuración SMTP (opcional)
    
    Raises:
        EmailSecurityException: Para errores de seguridad
        smtplib.SMTPException: Para errores SMTP
    """
    try:
        # Obtener configuración segura
        if config is None:
            config = get_email_config()
        
        # Crear mensaje seguro
        message = create_secure_message(
            sender=config.sender_email,
            recipients=recipients,
            subject=subject,
            body=body,
            is_html=is_html
        )
        
        # Configurar SSL/TLS seguro
        ssl_context = ssl.create_default_context()
        if config.require_tls:
            ssl_context.minimum_version = ssl.TLSVersion.TLSv1_2
        
        # Enviar con protecciones
        with smtplib.SMTP(config.server, config.port, timeout=config.timeout) as server:
            server.ehlo()
            
            if config.require_tls:
                if not server.has_extn('starttls'):
                    raise EmailSecurityException("Servidor no soporta TLS")
                server.starttls(context=ssl_context)
                server.ehlo()  # Renegociar después de TLS
            
            server.login(config.username, config.password)
            server.sendmail(config.sender_email, recipients, message.as_string())
    
    except ssl.SSLError as e:
        raise EmailSecurityException(f"Fallo de seguridad SSL/TLS: {e}") from e
    except smtplib.SMTPException as e:
        raise EmailSecurityException(f"Error SMTP seguro: {e}") from e
    except Exception as e:
        raise EmailSecurityException(f"Error inesperado: {e}") from e

# Ejemplo de uso seguro
if __name__ == "__main__":
    try:
        # En producción, configurar estas variables de entorno:
        # SMTP_SERVER, SMTP_PORT, SMTP_USERNAME, SMTP_PASSWORD, EMAIL_SENDER
        
        send_secure_email(
            recipients=["destinatario_valido@example.com"],
            subject="Asunto seguro",
            body="Contenido del mensaje sanitizado",
            is_html=False
        )
        print("Email enviado con todas las protecciones de seguridad")
    except EmailSecurityException as e:
        print(f"Error de seguridad detectado: {e}")
    except Exception as e:
        print(f"Error inesperado: {e}")