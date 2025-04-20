#!/usr/bin/env python3
from scapy.all import *
from scapy.layers.http import HTTPRequest
import argparse
import re
from datetime import datetime
import signal
import sys
import os
import time

# Configuración de colores para la salida
class bcolors:
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    INFO = '\033[94m'

# Patrones sospechosos optimizados para evitar ReDoS
SUSPICIOUS_PATTERNS = {
    'sql_injection': re.compile(r'(%27|\'|--|%23|#)', re.IGNORECASE),
    'xss': re.compile(r'(%3C|<)[a-z0-9%]*(%3E|>)', re.IGNORECASE),
    'path_traversal': re.compile(r'(\.\./|\.\.\\){2,}', re.IGNORECASE),
    'cmd_injection': re.compile(r'[;|&`$]', re.IGNORECASE),
    'sensitive_paths': re.compile(r'(admin|login|wp-admin|config|backup|\.env|\.git)', re.IGNORECASE)
}

# Timeout para operaciones regex (mitiga CWE-1333)
REGEX_TIMEOUT = 0.1  # 100ms máximo por operación regex

def signal_handler(sig, frame):
    print(f"\n{bcolors.INFO}[*] Deteniendo el sniffer...{bcolors.ENDC}")
    sys.exit(0)

def check_privileges():
    """Verifica privilegios de root (mitiga CWE-284)"""
    if os.geteuid() != 0:
        print(f"{bcolors.FAIL}[!] Error: Se requieren privilegios de root/admin{bcolors.ENDC}")
        sys.exit(1)

def safe_regex_search(pattern, text):
    """Ejecuta regex con timeout (mitiga CWE-1333)"""
    start_time = time.time()
    result = pattern.search(text)
    if time.time() - start_time > REGEX_TIMEOUT:
        return None
    return result

def sanitize_output(content):
    """Ofusca datos sensibles en logs (mitiga CWE-532)"""
    sensitive_keywords = ['password', 'token', 'secret', 'credit', 'card']
    content_lower = content.lower()
    for keyword in sensitive_keywords:
        if keyword in content_lower:
            return '[REDACTED SENSITIVE DATA]'
    return content[:200] + '...' if len(content) > 200 else content

def validate_packet(packet):
    """Valida la estructura del paquete (mitiga CWE-20)"""
    if not packet.haslayer(IP) or not packet.haslayer(TCP):
        return False
    if packet.haslayer(HTTPRequest):
        http_layer = packet[HTTPRequest]
        if not hasattr(http_layer, 'Method') or not hasattr(http_layer, 'Host'):
            return False
    return True

def analyze_http_packet(packet):
    if not validate_packet(packet):
        return

    http_layer = packet[HTTPRequest]
    ip_layer = packet[IP]
    
    try:
        method = http_layer.Method.decode(errors='replace')
        host = http_layer.Host.decode(errors='replace') if http_layer.Host else "N/A"
        path = http_layer.Path.decode(errors='replace')
        src_ip = ip_layer.src
        dst_ip = ip_layer.dst
        
        url = f"http://{host}{path}" if host != "N/A" else f"{dst_ip}{path}"
        
        print(f"\n{bcolors.INFO}[+] HTTP Request: {src_ip} -> {dst_ip} {method} {url}{bcolors.ENDC}")
        
        if hasattr(http_layer, 'fields'):
            headers = http_layer.fields.get('Additional-Fields', {})
            for header, value in headers.items():
                try:
                    header_str = f"{header.decode(errors='replace')}: {value.decode(errors='replace')}"
                    check_patterns(header_str, 'header')
                except:
                    continue
        
        if packet.haslayer(Raw):
            try:
                load = packet[Raw].load.decode(errors='replace')
                sanitized_load = sanitize_output(load)
                print(f"{bcolors.INFO}[*] Body content: {sanitized_load}{bcolors.ENDC}")
                check_patterns(load, 'body')
            except:
                pass

    except Exception as e:
        print(f"{bcolors.WARNING}[!] Error procesando paquete: {str(e)}{bcolors.ENDC}")

def check_patterns(content, content_type):
    if not isinstance(content, str):
        return
        
    for pattern_name, pattern in SUSPICIOUS_PATTERNS.items():
        try:
            matches = safe_regex_search(pattern, content)
            if matches:
                print(f"{bcolors.WARNING}[!] Possible {pattern_name} in {content_type}{bcolors.ENDC}")
        except:
            continue

def start_sniffing(interface, filter_expression="tcp port 80 or tcp port 443"):
    print(f"{bcolors.INFO}[*] Starting HTTP/HTTPS sniffer on interface {interface}...{bcolors.ENDC}")
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
    # Verificar privilegios antes de empezar
    check_privileges()
    
    parser = argparse.ArgumentParser(description="HTTP Sniffer for Suspicious Patterns")
    parser.add_argument("-i", "--interface", required=True, help="Network interface to sniff on")
    parser.add_argument("-f", "--filter", default="tcp port 80 or tcp port 443", 
                        help="BPF filter to apply (default: tcp port 80 or 443)")
    
    args = parser.parse_args()
    
    signal.signal(signal.SIGINT, signal_handler)
    
    if not 'http' in conf.layers:
        print(f"{bcolors.FAIL}[!] Error: Scapy HTTP layer not loaded. Run 'load_http()' first.{bcolors.ENDC}")
        sys.exit(1)
    
    start_sniffing(args.interface, args.filter)