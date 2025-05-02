import csv
import os

def calcular_promedios(archivo_entrada, archivo_salida):
    estudiantes_promedio = []

    try:
        # Verifica que el archivo de entrada exista
        if not os.path.exists(archivo_entrada):
            raise FileNotFoundError(f"El archivo '{archivo_entrada}' no existe.")

        with open(archivo_entrada, newline='', encoding='utf-8') as csvfile:
            lector = csv.DictReader(csvfile)
            for fila in lector:
                try:
                    nombre = fila['nombre']
                    nota1 = float(fila['nota1'])
                    nota2 = float(fila['nota2'])
                    nota3 = float(fila['nota3'])
                    promedio = round((nota1 + nota2 + nota3) / 3, 2)
                    estudiantes_promedio.append({'nombre': nombre, 'promedio': promedio})
                except ValueError as ve:
                    print(f"Error de formato en notas para {fila.get('nombre', 'desconocido')}: {ve}")
                except KeyError as ke:
                    print(f"Columna faltante en archivo CSV: {ke}")
                    return

        # Escribir resultados en archivo de salida
        with open(archivo_salida, 'w', newline='', encoding='utf-8') as csvfile:
            campos = ['nombre', 'promedio']
            escritor = csv.DictWriter(csvfile, fieldnames=campos)
            escritor.writeheader()
            escritor.writerows(estudiantes_promedio)
        print(f"Archivo generado exitosamente en: {archivo_salida}")

    except FileNotFoundError as e:
        print(f"Error: {e}")
    except Exception as e:
        print(f"Ocurrió un error inesperado: {e}")

# Ejemplo de uso
entrada = input("Ruta del archivo CSV de entrada: ")
salida = input("Ruta donde guardar el archivo CSV de salida: ")
calcular_promedios(entrada, salida)
