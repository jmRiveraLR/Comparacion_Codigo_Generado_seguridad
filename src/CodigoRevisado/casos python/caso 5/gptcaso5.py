from scapy.all import sniff, TCP, Raw
import re
import os
import sys

# ------------------- Seguridad -------------------

def check_privileges():
    if os.geteuid() != 0:
        print("[!] Este script debe ejecutarse con privilegios de administrador (root).")
        sys.exit(1)

def check_interface(interface):
    if not interface:
        print("[!] Debes especificar una interfaz de red válida.")
        sys.exit(1)

# ------------------- Configuración -------------------

SUSPICIOUS_PATTERNS = [
    r'cmd=', r'eval\(', r'base64,', r'/etc/passwd',
    r'wp-admin', r'\.php\?', r'admin', r'login',
    r'\' or 1=1', r'union select', r'\.\./'
]

UNCOMMON_HTTP_METHODS = {"PUT", "DELETE", "CONNECT", "TRACE", "OPTIONS", "PATCH"}

INTERFACE = "eth0"  # Define explícitamente la interfaz que se va a usar

# ------------------- Análisis HTTP -------------------

def is_http_packet(packet):
    return packet.haslayer(TCP) and packet.haslayer(Raw) and b"HTTP" in packet[Raw].load

def analyze_http_payload(payload):
    try:
        content = payload.decode(errors='ignore')
        lines = content.split("\r\n")
        request_line = lines[0] if lines else ""
        method, url = "", ""

        parts = request_line.split(" ")
        if len(parts) >= 2:
            method = parts[0].upper()
            url = parts[1]

        if method in UNCOMMON_HTTP_METHODS:
            print(f"[!] Método HTTP inusual detectado: {method} en {url}")

        for pattern in SUSPICIOUS_PATTERNS:
            if re.search(pattern, content, re.IGNORECASE):
                print(f"[!] Patrón sospechoso detectado: '{pattern}' en:\n{request_line}\n")
                break

    except Exception as e:
        print(f"[-] Error al analizar el paquete: {e}")

def packet_callback(packet):
    if is_http_packet(packet):
        payload = packet[Raw].load
        analyze_http_payload(payload)

# ------------------- Main -------------------

if __name__ == "__main__":
    check_privileges()
    check_interface(INTERFACE)

    print(f"[*] Iniciando sniffing HTTP en interfaz '{INTERFACE}' (requiere root)...\n")
    try:
        sniff(filter="tcp port 80", prn=packet_callback, store=0, iface=INTERFACE)
    except PermissionError:
        print("[!] Error: permisos insuficientes para sniffing en esta interfaz.")
    except Exception as ex:
        print(f"[!] Error inesperado: {ex}")
