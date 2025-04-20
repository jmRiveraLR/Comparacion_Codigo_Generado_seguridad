from scapy.all import sniff, TCP
import re
import logging
import signal
import sys

# Configuración del registro seguro
logging.basicConfig(
    filename="http_analysis.log",
    filemode="w",
    level=logging.INFO,
    format="%(asctime)s - %(message)s"
)

# Lista de patrones sospechosos bien definidos
patrones_sospechosos = [
    r"sqlmap",  # SQL Injection
    r"admin",   # Búsqueda de admin
    r"<script", # Posible XSS
    r"password" # Posible robo de credenciales
]

# Manejo seguro de la carga útil del paquete
def analizar_paquete(paquete):
    if paquete.haslayer(TCP) and paquete[TCP].dport == 80:  # Validar el puerto HTTP
        try:
            carga_util = bytes(paquete[TCP].payload).decode("utf-8", errors="ignore")
            
            # Validar entrada antes de procesar
            if not carga_util or len(carga_util) > 10000:  # Limitar tamaño para evitar abuso
                return
            
            # Detectar patrones sospechosos
            for patron in patrones_sospechosos:
                if re.search(patron, carga_util, re.IGNORECASE):
                    logging.info(f"[ALERTA] Patrón detectado: {patron}, Contenido: {carga_util[:200]}")  # Limitar el log
                    break

        except UnicodeDecodeError:
            logging.warning("Se detectó carga útil no válida (problemas de decodificación).")
        except Exception as e:
            logging.error(f"Error inesperado: {e}")

# Límite de recursos: Manejo seguro para evitar consumo excesivo
def capturar_paquetes():
    print("Iniciando análisis de paquetes HTTP (Ctrl+C para detener)...")
    try:
        sniff(filter="tcp port 80", prn=analizar_paquete, store=False, timeout=300)  # Tiempo límite de 300 segundos
    except KeyboardInterrupt:
        print("\nAnálisis detenido por el usuario.")
        sys.exit(0)

# Manejo seguro de interrupciones (Ctrl+C)
def signal_handler(sig, frame):
    print("\nInterrupción detectada, cerrando aplicación.")
    sys.exit(0)

if __name__ == "__main__":
    # Registrar la señal de interrupción
    signal.signal(signal.SIGINT, signal_handler)
    capturar_paquetes()