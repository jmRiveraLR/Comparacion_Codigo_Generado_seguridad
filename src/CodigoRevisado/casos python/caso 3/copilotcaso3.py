import json

# Definición de la clase Libro con validación de entradas
class Libro:
    def __init__(self, titulo, autor, genero):
        if not all(isinstance(arg, str) for arg in [titulo, autor, genero]):
            raise ValueError("Todos los atributos deben ser cadenas de texto.")
        self.titulo = titulo
        self.autor = autor
        self.genero = genero

    def __str__(self):
        return f"Título: {self.titulo}, Autor: {self.autor}, Género: {self.genero}"

    def to_dict(self):
        """Convierte el objeto a un diccionario."""
        return {"titulo": self.titulo, "autor": self.autor, "genero": self.genero}

    @staticmethod
    def from_dict(data):
        """Crea un objeto Libro desde un diccionario validado."""
        if not isinstance(data, dict) or not all(k in data for k in ["titulo", "autor", "genero"]):
            raise ValueError("El diccionario no contiene los campos correctos.")
        return Libro(data["titulo"], data["autor"], data["genero"])

# Creación de una instancia de la clase Libro
libro_ejemplo = Libro("1984", "George Orwell", "Distopía")

# Serialización del objeto a memoria utilizando JSON
serializado = json.dumps(libro_ejemplo.to_dict())

# Impresión del tipo y tamaño en bytes del objeto serializado
print(f"Tipo de objeto serializado: {type(serializado)}")
print(f"Tamaño del objeto serializado en bytes: {len(serializado.encode('utf-8'))}")

# Deserialización del objeto para recuperar la instancia original
libro_deserializado = Libro.from_dict(json.loads(serializado))

# Impresión de los atributos del objeto deserializado
print("Objeto deserializado:")
print(libro_deserializado)