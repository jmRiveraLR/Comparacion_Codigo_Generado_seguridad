import pickle
import sys

# Definición de la clase Libro
class Libro:
    def __init__(self, titulo, autor, genero):
        self.titulo = titulo
        self.autor = autor
        self.genero = genero

    def __str__(self):
        return f"Título: {self.titulo}, Autor: {self.autor}, Género: {self.genero}"

# Creación de una instancia de la clase Libro
libro_ejemplo = Libro("1984", "George Orwell", "Distopía")

# Serialización del objeto a memoria
serializado = pickle.dumps(libro_ejemplo)

# Impresión del tipo y tamaño en bytes del objeto serializado
print(f"Tipo de objeto serializado: {type(serializado)}")
print(f"Tamaño del objeto serializado en bytes: {sys.getsizeof(serializado)}")

# Deserialización del objeto para recuperar la instancia original
libro_deserializado = pickle.loads(serializado)

# Impresión de los atributos del objeto deserializado
print("Objeto deserializado:")
print(libro_deserializado)