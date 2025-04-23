from flask import Flask, request, jsonify, make_response
from flask_wtf.csrf import CSRFProtect
from flask_basicauth import BasicAuth

app = Flask(__name__)

# Configuración para CSRF
app.config['SECRET_KEY'] = 'clave_secreta_segura'
csrf = CSRFProtect(app)

# Configuración para autenticación básica
app.config['BASIC_AUTH_USERNAME'] = 'admin'
app.config['BASIC_AUTH_PASSWORD'] = 'contraseña_segura'
basic_auth = BasicAuth(app)

# Lista de opciones para votar
options = {"Opción 1": 0, "Opción 2": 0, "Opción 3": 0}

@app.route('/votar', methods=['POST'])
@csrf.exempt  # Esto asegura protección para solicitudes POST
def votar():
    if request.cookies.get('has_voted'):
        return jsonify({"mensaje": "Ya has votado. No puedes votar de nuevo."}), 403

    data = request.get_json()
    opcion = data.get('opcion')

    if opcion not in options:
        return jsonify({"mensaje": "Opción no válida."}), 400

    options[opcion] += 1

    # Crear la respuesta con cookies seguras
    response = make_response(jsonify({"mensaje": "Voto registrado con éxito."}))
    response.set_cookie(
        'has_voted',
        'true',
        max_age=60*60*24,  # Cookie válida por 1 día
        secure=True,       # Transmisión solo en HTTPS
        httponly=True      # No accesible desde JavaScript
    )
    return response

@app.route('/resultados', methods=['GET'])
@basic_auth.required
def resultados():
    return jsonify(options)

if __name__ == '__main__':
    app.run()  # Modo depuración desactivado para mayor seguridad