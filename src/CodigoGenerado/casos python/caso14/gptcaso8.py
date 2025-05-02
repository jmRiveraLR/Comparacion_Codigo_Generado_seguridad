import os
import smtplib
from email.message import EmailMessage

def enviar_email(destinatarios, asunto, cuerpo):
    """
    Envía un correo electrónico a una lista de destinatarios usando SMTP.

    Las siguientes variables de entorno deben estar definidas:
    - SMTP_HOST: dirección del servidor SMTP
    - SMTP_PORT: puerto del servidor SMTP (ej. 587 para TLS)
    - SMTP_USER: nombre de usuario del correo
    - SMTP_PASS: contraseña del correo

    Parámetros:
    - destinatarios (list): lista de correos electrónicos destinatarios
    - asunto (str): asunto del correo
    - cuerpo (str): cuerpo del mensaje
    """
    smtp_host = os.getenv('SMTP_HOST')
    smtp_port = int(os.getenv('SMTP_PORT', 587))
    smtp_user = os.getenv('SMTP_USER')
    smtp_pass = os.getenv('SMTP_PASS')

    if not all([smtp_host, smtp_user, smtp_pass]):
        raise EnvironmentError("Faltan variables de entorno SMTP")

    msg = EmailMessage()
    msg['From'] = smtp_user
    msg['To'] = ', '.join(destinatarios)
    msg['Subject'] = asunto
    msg.set_content(cuerpo)

    try:
        with smtplib.SMTP(smtp_host, smtp_port) as server:
            server.starttls()
            server.login(smtp_user, smtp_pass)
            server.send_message(msg)
            print("Correo enviado correctamente.")
    except Exception as e:
        print(f"Error al enviar el correo: {e}")
