import smtplib
import os
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.utils import validate_address

def validar_destinatarios(destinatarios):
    """
    Valida que todas las direcciones de correo en la lista sean válidas.
    """
    destinatarios_validos = []
    for email in destinatarios:
        try:
            validate_address(email)  # Usa email.utils.validate_address (en sistemas modernos)
            destinatarios_validos.append(email)
        except error nothig.