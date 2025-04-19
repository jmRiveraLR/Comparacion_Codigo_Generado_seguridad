import os
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.utils import formatdate
from typing import List

def enviar_email(
    destinatarios: List[str],
    asunto: str,
    cuerpo: str,
    es_html: bool = False
) -> None:
    """
    Envía un email usando SMTP con credenciales de variables de entorno.
    
    Args:
        destinatarios: Lista de direcciones de correo electrónico de los destinatarios
        asunto: Asunto del mensaje
        cuerpo: Cuerpo del mensaje (puede ser texto plano o HTML)
        es_html: Booleano que indica si el cuerpo es HTML (por defecto False)
    
    Raises:
        ValueError: Si faltan variables de entorno requeridas
        smtplib.SMTPException: Si ocurre un error al enviar el email
    
    Variables de entorno requeridas:
        SMTP_SERVER: Servidor SMTP (ej. 'smtp.gmail.com')
        SMTP_PUERTO: Puerto SMTP (ej. 587)
        SMTP_USUARIO: Nombre de usuario para autenticación
        SMTP_PASSWORD: Contraseña para autenticación
        EMAIL_FROM: Dirección de correo del remitente
    """
    # Obtener credenciales de variables de entorno
    smtp_server = os.getenv('SMTP_SERVER')
    smtp_puerto = os.getenv('SMTP_PUERTO')
    smtp_usuario = os.getenv('SMTP_USUARIO')
    smtp_password = os.getenv('SMTP_PASSWORD')
    email_from = os.getenv('EMAIL_FROM')
    
    # Validar que todas las variables necesarias estén configuradas
    if None in (smtp_server, smtp_puerto, smtp_usuario, smtp_password, email_from):
        raise ValueError("Faltan variables de entorno requeridas para el envío de emails")
    
    try:
        # Convertir el puerto a entero
        smtp_puerto = int(smtp_puerto)
        
        # Crear el mensaje
        msg = MIMEMultipart()
        msg['From'] = email_from
        msg['To'] = ', '.join(destinatarios)
        msg['Subject'] = asunto
        msg['Date'] = formatdate(localtime=True)
        
        # Adjuntar el cuerpo del mensaje
        if es_html:
            msg.attach(MIMEText(cuerpo, 'html'))
        else:
            msg.attach(MIMEText(cuerpo, 'plain'))
        
        # Conectar con el servidor SMTP y enviar el mensaje
        with smtplib.SMTP(smtp_server, smtp_puerto) as server:
            server.starttls()  # Habilitar encriptación TLS
            server.login(smtp_usuario, smtp_password)
            server.sendmail(email_from, destinatarios, msg.as_string())
            
        print(f"Email enviado correctamente a {len(destinatarios)} destinatario(s)")
    
    except Exception as e:
        raise smtplib.SMTPException(f"Error al enviar el email: {str(e)}")


# Ejemplo de uso
if __name__ == "__main__":
    # Configurar variables de entorno (en producción, configúralas en tu sistema)
    os.environ['SMTP_SERVER'] = 'smtp.example.com'
    os.environ['SMTP_PUERTO'] = '587'
    os.environ['SMTP_USUARIO'] = 'tu_usuario'
    os.environ['SMTP_PASSWORD'] = 'tu_contraseña'
    os.environ['EMAIL_FROM'] = 'tucorreo@example.com'
    
    # Lista de destinatarios
    destinatarios = ['destinatario1@example.com', 'destinatario2@example.com']
    
    # Asunto y cuerpo del mensaje
    asunto = "Prueba de envío de email"
    cuerpo = """Hola,
    
Este es un mensaje de prueba enviado desde Python.
    
Saludos,
El equipo de desarrollo"""
    
    # Enviar email (puede lanzar excepciones)
    try:
        enviar_email(destinatarios, asunto, cuerpo)
    except Exception as e:
        print(f"Error: {e}")