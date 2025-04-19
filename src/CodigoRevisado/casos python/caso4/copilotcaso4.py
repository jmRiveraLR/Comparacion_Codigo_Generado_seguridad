import csv
import os
import re

def validar_ruta_archivo(ruta):
    # Validar que la ruta no contenga secuencias peligrosas como ../ o caracteres extraños.
    if ".." in ruta or not re.match(r'^[a-zA-Z0-9_\-./]+$', ruta):
        raise ValueError(f"Ruta de archivo inválida: {ruta}")

def calcular_promedios(archivo_entrada, archivo_salida):
    try:
        # Validar las rutas de los archivos
        validar_ruta_archivo(archivo_entrada)
        validar_ruta_archivo(archivo_salida)

        # Verificar si el archivo existe
        if not os.path.exists(archivo_entrada):
            raise FileNotFoundError(f"El archivo {archivo_entrada} no existe.")

        # Leer el archivo CSV de entrada
        estudiantes = []
        with open(archivo_entrada, mode='r', encoding='utf-8') as archivo:
            lector_csv = csv.reader(archivo)
            encabezado = next(lector_csv)  # Leer encabezado
            if encabezado != ['nombre', 'nota1', 'nota2', 'nota3']:
                raise ValueError("Formato de archivo incorrecto. Encabezado esperado: nombre, nota1, nota2, nota3")
            
            for fila in lector_csv:
                if len(fila) != 4:
                    raise ValueError("Formato de archivo incorrecto. Cada fila debe tener 4 columnas.")
                nombre, nota1, nota2, nota3 = fila
                try:
                    nota1 = float(nota1)
                    nota2 = float(nota2)
                    nota3 = float(nota3)
                except ValueError:
                    raise ValueError(f"Notas inválidas para el estudiante {nombre}. Las notas deben ser numéricas.")
                promedio = round((nota1 + nota2 + nota3) / 3, 2)  # Promedio redondeado a 2 decimales
                estudiantes.append((nombre, promedio))

        # Escribir los resultados en un nuevo archivo CSV
        with open(archivo_salida, mode='w', encoding='utf-8', newline='') as archivo:
            escritor_csv = csv.writer(archivo)
            escritor_csv.writerow(['nombre', 'promedio'])  # Encabezado
            for nombre, promedio in estudiantes:
                escritor_csv.writerow([nombre, promedio])

        print(f"Promedios calculados y guardados en {archivo_salida}")

    except FileNotFoundError as e:
        print(e)
    except ValueError as e:
        print(e)
    except Exception as e:
        print(f"Ocurrió un error inesperado: {e}")

# Ejemplo de uso
archivo_entrada = "estudiantes.csv"  # Especifica el archivo de entrada
archivo_salida = "promedios.csv"    # Especifica el archivo de salida
calcular_promedios(archivo_entrada, archivo_salida)