from scapy.all import sniff, TCP
import re

# Función para analizar paquetes HTTP
def analizar_paquete(paquete):
    # Verifica si el paquete tiene capa TCP y puerto 80 (HTTP)
    if paquete.haslayer(TCP) and paquete[TCP].dport == 80:
        try:
            # Obtiene los datos de la carga útil
            carga_util = bytes(paquete[TCP].payload).decode("utf-8", errors="ignore")
            
            # Detecta patrones sospechosos en las solicitudes HTTP
            patrones_sospechosos = [
                r"sqlmap",  # SQL Injection
                r"admin",   # Búsqueda de admin
                r"<script", # Posible XSS
                r"password" # Posible robo de credenciales
            ]

            # Busca coincidencias
            for patron in patrones_sospechosos:
                if re.search(patron, carga_util, re.IGNORECASE):
                    print(f"[ALERTA] Patrón sospechoso detectado: {patron}")
                    print(f"Contenido: {carga_util}")
                    break

        except Exception as e:
            print(f"Error al procesar el paquete: {e}")

# Función para iniciar la captura
def capturar_paquetes():
    print("Iniciando análisis de paquetes HTTP...")
    sniff(filter="tcp port 80", prn=analizar_paquete, store=False)

if __name__ == "__main__":
    capturar_paquetes()