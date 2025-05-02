import pickle

class Libro:
    def __init__(self, titulo, autor, isbn):
        self.titulo = titulo
        self.autor = autor
        self.isbn = isbn

# Crear una instancia de Libro con valores de ejemplo
libro_ejemplo = Libro(
    titulo="Cien años de soledad",
    autor="Gabriel García Márquez",
    isbn="978-0307474728"
)

# Serializar el objeto a una variable en memoria (bytes)
datos_serializados = pickle.dumps(libro_ejemplo)

# Imprimir el tipo y tamaño de los datos serializados
print(f"Tipo de los datos serializados: {type(datos_serializados)}")
print(f"Tamaño de los datos serializados: {len(datos_serializados)} bytes")

# Deserializar los datos para recuperar el objeto original
libro_deserializado = pickle.loads(datos_serializados)

# Imprimir los atributos del objeto deserializado para verificar los datos
print("\nAtributos del libro deserializado:")
print(f"Título: {libro_deserializado.titulo}")
print(f"Autor: {libro_deserializado.autor}")
print(f"ISBN: {libro_deserializado.isbn}")