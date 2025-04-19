#!/usr/bin/env python3
from scapy.all import *
from scapy.layers.http import HTTPRequest
import argparse
import re
from datetime import datetime
import signal
import sys

# Configuración de colores para la salida
class bcolors:
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    INFO = '\033[94m'

# Patrones sospechosos para detectar
SUSPICIOUS_PATTERNS = {
    'sql_injection': re.compile(r'(\%27)|(\')|(--)|(\%23)|(#)|(\%2D\%2D)', re.IGNORECASE),
    'xss': re.compile(r'((\%3C)|<)((\%2F)|\/)*[a-z0-9\%]+((\%3E)|>)', re.IGNORECASE),
    'path_traversal': re.compile(r'((\.\.\/)|(\.\.\\))+', re.IGNORECASE),
    'cmd_injection': re.compile(r'(\|)|(;)|(\&\&)|(\|\|)|(\`)|(\$\(|\()', re.IGNORECASE),
    'sensitive_paths': re.compile(r'(admin|login|wp-admin|config|backup|\.env|\.git)', re.IGNORECASE),
    'unusual_headers': re.compile(r'(x-forwarded-for|x-attackip|x-ratproxy-ip)', re.IGNORECASE)
}

def signal_handler(sig, frame):
    print(f"\n{bcolors.INFO}[*] Deteniendo el sniffer...{bcolors.ENDC}")
    sys.exit(0)

def analyze_http_packet(packet):
    if packet.haslayer(HTTPRequest):
        http_layer = packet[HTTPRequest]
        ip_layer = packet[IP]
        
        # Extraer información básica
        method = http_layer.Method.decode()
        host = http_layer.Host.decode() if http_layer.Host else "N/A"
        path = http_layer.Path.decode()
        src_ip = ip_layer.src
        dst_ip = ip_layer.dst
        
        # Construir URL completa
        url = f"http://{host}{path}" if host != "N/A" else f"{dst_ip}{path}"
        
        print(f"\n{bcolors.INFO}[+] HTTP Request: {src_ip} -> {dst_ip} {method} {url}{bcolors.ENDC}")
        
        # Analizar headers
        if hasattr(http_layer, 'fields'):
            headers = http_layer.fields.get('Additional-Fields', {})
            for header, value in headers.items():
                header_str = f"{header.decode()}: {value.decode()}"
                check_patterns(header_str, 'header')
        
        # Analizar cuerpo si existe (para POST)
        if packet.haslayer(Raw):
            load = packet[Raw].load.decode(errors='ignore')
            print(f"{bcolors.INFO}[*] Body content (truncated): {load[:200]}...{bcolors.ENDC}")
            check_patterns(load, 'body')

def check_patterns(content, content_type):
    for pattern_name, pattern in SUSPICIOUS_PATTERNS.items():
        matches = pattern.findall(content) if isinstance(content, str) else []
        if matches:
            print(f"{bcolors.WARNING}[!] Possible {pattern_name} in {content_type}: {matches}{bcolors.ENDC}")

def start_sniffing(interface, filter_expression="tcp port 80"):
    print(f"{bcolors.INFO}[*] Starting HTTP sniffer on interface {interface}...{bcolors.ENDC}")
    print(f"{bcolors.INFO}[*] Filter: {filter_expression}{bcolors.ENDC}")
    print(f"{bcolors.INFO}[*] Press Ctrl+C to stop{bcolors.ENDC}")
    
    try:
        sniff(iface=interface, filter=filter_expression, 
              prn=analyze_http_packet, store=False)
    except PermissionError:
        print(f"{bcolors.FAIL}[!] Error: Need root privileges to sniff packets{bcolors.ENDC}")
        sys.exit(1)
    except Exception as e:
        print(f"{bcolors.FAIL}[!] Error: {e}{bcolors.ENDC}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="HTTP Sniffer for Suspicious Patterns")
    parser.add_argument("-i", "--interface", required=True, help="Network interface to sniff on")
    parser.add_argument("-f", "--filter", default="tcp port 80", 
                        help="BPF filter to apply (default: tcp port 80)")
    
    args = parser.parse_args()
    
    # Configurar handler para Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)
    
    # Verificar que Scapy tenga el módulo http
    if not 'http' in conf.layers:
        print(f"{bcolors.FAIL}[!] Error: Scapy HTTP layer not loaded. Run 'load_http()' first.{bcolors.ENDC}")
        sys.exit(1)
    
    start_sniffing(args.interface, args.filter)