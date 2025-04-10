import csv
import os
import logging
from pathlib import Path

# Configurar logging
logging.basicConfig(filename='registro_errores.log',
                    level=logging.INFO,
                    format='%(asctime)s - %(levelname)s - %(message)s')

def validar_ruta_csv(ruta, debe_existir=True):
    path = Path(ruta)
    if path.suffix.lower() != '.csv':
        raise ValueError("El archivo debe tener extensión .csv")
    if debe_existir and not path.exists():
        raise FileNotFoundError(f"El archivo '{ruta}' no existe.")
    return path.resolve()

def validar_encabezados(encabezados_reales, encabezados_esperados):
    faltantes = [h for h in encabezados_esperados if h not in encabezados_reales]
    if faltantes:
        raise ValueError(f"Faltan columnas requeridas: {faltantes}")

def calcular_promedios(archivo_entrada, archivo_salida):
    estudiantes_promedio = []
    encabezados_esperados = ['nombre', 'nota1', 'nota2', 'nota3']

    try:
        ruta_entrada = validar_ruta_csv(archivo_entrada, debe_existir=True)
        ruta_salida = validar_ruta_csv(archivo_salida, debe_existir=False)

        with open(ruta_entrada, newline='', encoding='utf-8') as csvfile:
            lector = csv.DictReader(csvfile)
            validar_encabezados(lector.fieldnames, encabezados_esperados)

            for fila in lector:
                try:
                    nombre = fila['nombre']
                    nota1 = float(fila['nota1'])
                    nota2 = float(fila['nota2'])
                    nota3 = float(fila['nota3'])
                    promedio = round((nota1 + nota2 + nota3) / 3, 2)
                    estudiantes_promedio.append({'nombre': nombre, 'promedio': promedio})
                except (ValueError, TypeError) as ve:
                    logging.warning(f"Error al procesar notas de {fila.get('nombre', 'desconocido')}: {ve}")

        with open(ruta_salida, 'w', newline='', encoding='utf-8') as csvfile:
            campos = ['nombre', 'promedio']
            escritor = csv.DictWriter(csvfile, fieldnames=campos)
            escritor.writeheader()
            escritor.writerows(estudiantes_promedio)

        print(f"Archivo generado exitosamente en: {ruta_salida}")

    except FileNotFoundError as e:
        logging.error(e)
        print(f"Error: {e}")
    except ValueError as e:
        logging.error(e)
        print(f"Error de validación: {e}")
    except Exception as e:
        logging.exception("Error inesperado")
        print(f"Ocurrió un error inesperado: {e}")

# Ejemplo de uso
if __name__ == "__main__":
    entrada = input("Ruta del archivo CSV de entrada: ")
    salida = input("Ruta donde guardar el archivo CSV de salida: ")
    calcular_promedios(entrada, salida)
