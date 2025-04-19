from scapy.all import sniff, TCP, Raw
import re

# Palabras clave sospechosas en la URL o contenido HTTP
SUSPICIOUS_PATTERNS = [
    r'cmd=', r'eval\(', r'base64,', r'/etc/passwd',
    r'wp-admin', r'\.php\?', r'admin', r'login',
    r'\' or 1=1', r'union select', r'\.\./'
]

# Métodos HTTP sospechosos o poco comunes
UNCOMMON_HTTP_METHODS = {"PUT", "DELETE", "CONNECT", "TRACE", "OPTIONS", "PATCH"}

def is_http_packet(packet):
    return packet.haslayer(TCP) and packet.haslayer(Raw) and b"HTTP" in packet[Raw].load

def analyze_http_payload(payload):
    try:
        lines = payload.decode(errors='ignore').split("\r\n")
        request_line = lines[0] if lines else ""
        method = request_line.split(" ")[0]
        url = request_line.split(" ")[1] if len(request_line.split(" ")) > 1 else ""

        # Método HTTP inusual
        if method.upper() in UNCOMMON_HTTP_METHODS:
            print(f"[!] Método HTTP inusual detectado: {method} en {url}")

        # Búsqueda de patrones sospechosos en la URL o headers
        for pattern in SUSPICIOUS_PATTERNS:
            if re.search(pattern, payload.decode(errors='ignore'), re.IGNORECASE):
                print(f"[!] Patrón sospechoso detectado: '{pattern}' en payload:\n{payload.decode(errors='ignore')}\n")
                break
    except Exception as e:
        print(f"Error al analizar el paquete: {e}")

def packet_callback(packet):
    if is_http_packet(packet):
        payload = packet[Raw].load
        analyze_http_payload(payload)

if __name__ == "__main__":
    print("[*] Iniciando sniffing HTTP en red local (requiere privilegios de root)...\n")
    sniff(filter="tcp port 80", prn=packet_callback, store=0)
