import csv
import os

def calcular_promedios_estudiantes(archivo_entrada, archivo_salida, ubicacion_salida):
    """
    Calcula los promedios de estudiantes a partir de un archivo CSV y guarda los resultados en otro archivo.
    
    Args:
        archivo_entrada (str): Nombre del archivo CSV de entrada
        archivo_salida (str): Nombre del archivo CSV de salida
        ubicacion_salida (str): Ruta donde se guardará el archivo de salida
        
    Returns:
        bool: True si la operación fue exitosa, False si hubo algún error
    """
    
    # Verificar si el archivo de entrada existe
    if not os.path.exists(archivo_entrada):
        print(f"Error: El archivo '{archivo_entrada}' no existe.")
        return False
    
    try:
        # Leer el archivo CSV de entrada
        with open(archivo_entrada, mode='r', newline='', encoding='utf-8') as archivo:
            lector_csv = csv.reader(archivo)
            encabezados = next(lector_csv)  # Leer la primera fila (encabezados)
            
            # Verificar el formato del archivo
            if len(encabezados) < 4 or encabezados[0].lower() != 'nombre' or \
               not all(f'nota{i}' in encabezados[i].lower() for i in range(1, 4)):
                print("Error: El archivo no tiene el formato esperado (nombre, nota1, nota2, nota3).")
                return False
            
            estudiantes = []
            for fila_num, fila in enumerate(lector_csv, start=2):
                try:
                    nombre = fila[0]
                    notas = [float(nota) for nota in fila[1:4]]
                    
                    # Verificar que las notas estén en el rango 0-20 (opcional)
                    if any(nota < 0 or nota > 20 for nota in notas):
                        print(f"Advertencia: Nota fuera de rango (0-20) para {nombre} en fila {fila_num}")
                    
                    promedio = sum(notas) / len(notas)
                    estudiantes.append({'nombre': nombre, 'promedio': promedio})
                except (ValueError, IndexError) as e:
                    print(f"Error en fila {fila_num}: Datos inválidos - {str(e)}")
                    return False
        
        # Crear la ruta completa de salida si no existe
        os.makedirs(ubicacion_salida, exist_ok=True)
        ruta_completa = os.path.join(ubicacion_salida, archivo_salida)
        
        # Escribir el archivo CSV de salida
        with open(ruta_completa, mode='w', newline='', encoding='utf-8') as archivo:
            escritor_csv = csv.writer(archivo)
            escritor_csv.writerow(['Nombre', 'Promedio'])
            for estudiante in estudiantes:
                escritor_csv.writerow([estudiante['nombre'], round(estudiante['promedio'], 2)])
        
        print(f"Proceso completado. Resultados guardados en: {ruta_completa}")
        return True
    
    except Exception as e:
        print(f"Error inesperado: {str(e)}")
        return False


# Ejemplo de uso
if __name__ == "__main__":
    # Configuración
    archivo_entrada = 'estudiantes.csv'  # Nombre del archivo de entrada
    archivo_salida = 'promedios_estudiantes.csv'  # Nombre del archivo de salida
    ubicacion_salida = './resultados'  # Carpeta donde se guardará el resultado
    
    # Ejecutar la función
    calcular_promedios_estudiantes(archivo_entrada, archivo_salida, ubicacion_salida)