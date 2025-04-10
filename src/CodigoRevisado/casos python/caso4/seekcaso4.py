import csv
import os
import re
from pathlib import Path

# Constantes de seguridad
ALLOWED_DIR = "./safe_dir"  # Directorio permitido para salida
SAFE_FILENAME_PATTERN = re.compile(r'^[a-zA-Z0-9_\-]+\.csv$')
SAFE_NAME_PATTERN = re.compile(r'^[a-zA-ZáéíóúÁÉÍÓÚñÑ\s]+$')

def sanitize_path(user_path, base_dir=ALLOWED_DIR):
    """Previene Path Traversal (CWE-22) y controla rutas (CWE-73)"""
    try:
        base_dir = os.path.abspath(base_dir)
        user_path = os.path.normpath(user_path)
        full_path = os.path.abspath(os.path.join(base_dir, user_path))
        
        if not os.path.commonpath([base_dir, full_path]) == base_dir:
            raise ValueError("Intento de acceso a ruta no permitida")
        return full_path
    except (ValueError, TypeError) as e:
        raise ValueError(f"Ruta inválida: {str(e)}")

def validate_filename(filename):
    """Valida nombres de archivo seguros (CWE-73)"""
    if not SAFE_FILENAME_PATTERN.match(filename):
        raise ValueError("Nombre de archivo no permitido")
    return filename

def validate_student_name(name):
    """Valida nombres seguros (CWE-20, CWE-79)"""
    if not SAFE_NAME_PATTERN.match(name):
        raise ValueError("Nombre contiene caracteres no permitidos")
    return name.strip()

def validate_grade(grade):
    """Valida notas seguras (CWE-20)"""
    try:
        grade = float(grade)
        if not 0 <= grade <= 20:
            raise ValueError("Nota fuera de rango (0-20)")
        return grade
    except ValueError:
        raise ValueError("Nota debe ser numérica")

def calcular_promedios_estudiantes(archivo_entrada, archivo_salida, ubicacion_salida=ALLOWED_DIR):
    # Validación de rutas y nombres (CWE-22, CWE-73)
    try:
        ubicacion_salida = sanitize_path(ubicacion_salida)
        archivo_salida = validate_filename(archivo_salida)
        ruta_salida = os.path.join(ubicacion_salida, archivo_salida)
        
        # Crear directorio seguro con permisos (CWE-276)
        os.makedirs(ubicacion_salida, mode=0o700, exist_ok=True)
        
        # Validar existencia archivo entrada
        if not os.path.isfile(archivo_entrada):
            raise FileNotFoundError(f"Archivo no encontrado: {archivo_entrada}")

        estudiantes = []
        
        # Procesar archivo CSV con manejo seguro (CWE-20, CWE-79)
        with open(archivo_entrada, mode='r', encoding='utf-8') as f:
            reader = csv.reader(f)
            try:
                headers = next(reader)
                if len(headers) < 4 or not all(h.lower().startswith(('nombre', 'nota')) for h in headers[:4]):
                    raise ValueError("Formato de encabezados inválido")
                
                for row_num, row in enumerate(reader, 2):
                    try:
                        nombre = validate_student_name(row[0])
                        notas = [validate_grade(row[i]) for i in range(1, 4)]
                        promedio = round(sum(notas) / len(notas), 2)
                        estudiantes.append([nombre, promedio])
                    except (ValueError, IndexError) as e:
                        print(f"Error fila {row_num}: {str(e)} - Se omite registro")
                        continue
            
            except csv.Error as e:
                raise ValueError(f"Error procesando CSV: {str(e)}")

        # Escribir archivo de salida con permisos seguros (CWE-276)
        with open(ruta_salida, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['Nombre', 'Promedio'])
            writer.writerows(estudiantes)
        
        # Asegurar permisos del archivo (CWE-276)
        os.chmod(ruta_salida, 0o600)
        
        print(f"Proceso completado. Resultados en: {ruta_salida}")
        return True
    
    except Exception as e:
        print(f"Error de seguridad: {str(e)}")
        return False

# Ejemplo de uso seguro
if __name__ == "__main__":
    try:
        calcular_promedios_estudiantes(
            archivo_entrada="estudiantes.csv",
            archivo_salida="promedios.csv",
            ubicacion_salida="./resultados"
        )
    except Exception as e:
        print(f"Error crítico: {str(e)}")