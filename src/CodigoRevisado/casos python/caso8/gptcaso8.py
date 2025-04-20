import os
import smtplib
import ssl
from email.message import EmailMessage
import logging

# Configuración de logging en lugar de imprimir errores
logging.basicConfig(filename='email_errors.log', level=logging.ERROR)

def enviar_email(destinatarios, asunto, cuerpo):
    """
    Envía un correo electrónico a una lista de destinatarios usando SMTP con seguridad reforzada.

    Requiere las siguientes variables de entorno:
    - SMTP_HOST
    - SMTP_PORT
    - SMTP_USER
    - SMTP_PASS
    """
    smtp_host = os.getenv('SMTP_HOST')
    smtp_port = int(os.getenv('SMTP_PORT', 587))
    smtp_user = os.getenv('SMTP_USER')
    smtp_pass = os.getenv('SMTP_PASS')

    if not smtp_host or not smtp_user or not smtp_pass:
        logging.error("Faltan variables de entorno requeridas para SMTP.")
        raise EnvironmentError("Configuración incompleta de SMTP")

    msg = EmailMessage()
    msg['From'] = smtp_user
    msg['To'] = ', '.join(destinatarios)
    msg['Subject'] = asunto
    msg.set_content(cuerpo)

    try:
        context = ssl.create_default_context()
        with smtplib.SMTP(smtp_host, smtp_port) as server:
            server.starttls(context=context)  # Validación segura de TLS
            server.login(smtp_user, smtp_pass)
            server.send_message(msg)
    except Exception as e:
        logging.error("Error al enviar el correo", exc_info=True)
        raise RuntimeError("Error en el envío de correo, ver log para más detalles.")
