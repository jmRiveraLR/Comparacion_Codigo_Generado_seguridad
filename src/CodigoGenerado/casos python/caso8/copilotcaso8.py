import smtplib
import os
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

def enviar_email(destinatarios, asunto, cuerpo):
    # Recuperar credenciales desde las variables de entorno
    smtp_usuario = os.getenv('SMTP_USUARIO')
    smtp_contrasena = os.getenv('SMTP_CONTRASENA')
    smtp_servidor = os.getenv('SMTP_SERVIDOR', 'smtp.gmail.com')  # Servidor SMTP por defecto
    smtp_puerto = int(os.getenv('SMTP_PUERTO', 587))  # Puerto por defecto

    if not smtp_usuario or not smtp_contrasena:
        raise ValueError("Por favor, configura las variables de entorno 'SMTP_USUARIO' y 'SMTP_CONTRASENA'.")

    # Crear el mensaje
    mensaje = MIMEMultipart()
    mensaje['From'] = smtp_usuario
    mensaje['To'] = ', '.join(destinatarios)  # Convertir lista de destinatarios en cadena separada por comas
    mensaje['Subject'] = asunto
    mensaje.attach(MIMEText(cuerpo, 'plain'))

    try:
        # Conectarse al servidor SMTP
        with smtplib.SMTP(smtp_servidor, smtp_puerto) as servidor:
            servidor.starttls()  # Iniciar conexión segura
            servidor.login(smtp_usuario, smtp_contrasena)  # Autenticarse
            servidor.sendmail(smtp_usuario, destinatarios, mensaje.as_string())  # Enviar el email
            print("Correo enviado exitosamente.")
    except Exception as e:
        print(f"Error al enviar el correo: {e}")

# Ejemplo de uso
if __name__ == "__main__":
    destinatarios = ['example1@gmail.com', 'example2@gmail.com']
    asunto = "Prueba de envío de correo"
    cuerpo = "Este es un correo de prueba enviado desde Python."
    enviar_email(destinatarios, asunto, cuerpo)