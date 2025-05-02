import json

class Libro:
    def __init__(self, titulo, autor, editorial):
        # Validación básica de entrada
        if not all(isinstance(arg, str) and arg.strip() for arg in [titulo, autor, editorial]):
            raise ValueError("Todos los campos deben ser cadenas de texto no vacías.")

        self.titulo = titulo
        self.autor = autor
        self.editorial = editorial

    def to_dict(self):
        return {
            "titulo": self.titulo,
            "autor": self.autor,
            "editorial": self.editorial
        }

    @classmethod
    def from_dict(cls, data):
        return cls(
            titulo=data.get("titulo", ""),
            autor=data.get("autor", ""),
            editorial=data.get("editorial", "")
        )

# Crear una instancia del libro
libro_ejemplo = Libro("Cien Años de Soledad", "Gabriel García Márquez", "Editorial Sudamericana")

# Serialización segura con JSON
datos_serializados = json.dumps(libro_ejemplo.to_dict()).encode("utf-8")

# Imprimir tipo y tamaño de los datos serializados
print(f"Tipo de datos serializados: {type(datos_serializados)}")
print(f"Tamaño en bytes: {len(datos_serializados)}")

# Deserialización segura
datos_dict = json.loads(datos_serializados.decode("utf-8"))
libro_recuperado = Libro.from_dict(datos_dict)

# Imprimir los atributos del objeto deserializado
print("\nObjeto deserializado:")
print(f"Título: {libro_recuperado.titulo}")
print(f"Autor: {libro_recuperado.autor}")
print(f"Editorial: {libro_recuperado.editorial}")
