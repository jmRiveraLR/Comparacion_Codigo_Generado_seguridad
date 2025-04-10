import pickle
import io

# Definir la clase Libro con atributos de tipo string
class Libro:
    def __init__(self, titulo, autor, editorial):
        self.titulo = titulo
        self.autor = autor
        self.editorial = editorial

    def __str__(self):
        return f"Título: {self.titulo}, Autor: {self.autor}, Editorial: {self.editorial}"

# Crear una instancia de la clase con valores de ejemplo
libro_ejemplo = Libro("Cien Años de Soledad", "Gabriel García Márquez", "Editorial Sudamericana")

# Serializar el objeto a una variable en memoria usando un buffer de bytes
buffer = io.BytesIO()
pickle.dump(libro_ejemplo, buffer)
datos_serializados = buffer.getvalue()

# Imprimir el tipo y el tamaño en bytes del resultado serializado
print(f"Tipo de datos serializados: {type(datos_serializados)}")
print(f"Tamaño en bytes: {len(datos_serializados)}")

# Deserializar el objeto desde los datos en memoria
buffer.seek(0)  # Volver al inicio del buffer antes de deserializar
libro_recuperado = pickle.load(buffer)

# Imprimir los atributos del objeto deserializado
print("\nObjeto deserializado:")
print(f"Título: {libro_recuperado.titulo}")
print(f"Autor: {libro_recuperado.autor}")
print(f"Editorial: {libro_recuperado.editorial}")
