import json
from typing import Dict, Any

class Libro:
    def __init__(self, titulo: str, autor: str, isbn: str):
        self.titulo = titulo
        self.autor = autor
        self.isbn = isbn

    def to_dict(self) -> Dict[str, str]:
        """Convierte el objeto a un diccionario seguro para JSON."""
        return {
            "titulo": self.titulo,
            "autor": self.autor,
            "isbn": self.isbn
        }

    @classmethod
    def from_dict(cls, data: Dict[str, str]) -> 'Libro':
        """Crea un Libro desde un diccionario (validación básica)."""
        if not all(key in data for key in ["titulo", "autor", "isbn"]):
            raise ValueError("Datos incompletos o inválidos")
        return cls(data["titulo"], data["autor"], data["isbn"])

# --- Ejemplo seguro ---
# Crear instancia
libro_ejemplo = Libro(
    titulo="Cien años de soledad",
    autor="Gabriel García Márquez",
    isbn="978-0307474728"
)

# Serializar a JSON (en memoria)
datos_serializados = json.dumps(libro_ejemplo.to_dict())
print(f"Tipo: {type(datos_serializados)}, Tamaño: {len(datos_serializados)} bytes")

# Deserializar con validación
try:
    libro_deserializado = Libro.from_dict(json.loads(datos_serializados))
    print("\nAtributos del libro deserializado:")
    print(f"Título: {libro_deserializado.titulo}")
    print(f"Autor: {libro_deserializado.autor}")
    print(f"ISBN: {libro_deserializado.isbn}")
except (json.JSONDecodeError, ValueError) as e:
    print(f"Error de deserialización: {e}")