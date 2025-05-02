from flask import Flask, request, jsonify, make_response

app = Flask(__name__)

# Lista de opciones para votar
options = {"Opción 1": 0, "Opción 2": 0, "Opción 3": 0}

@app.route('/votar', methods=['POST'])
def votar():
    # Verificar si el usuario ya ha votado mediante cookies
    if request.cookies.get('has_voted'):
        return jsonify({"mensaje": "Ya has votado. No puedes votar de nuevo."}), 403

    data = request.get_json()
    opcion = data.get('opcion')

    if opcion not in options:
        return jsonify({"mensaje": "Opción no válida."}), 400

    # Incrementar el conteo de votos para la opción seleccionada
    options[opcion] += 1

    # Crear la respuesta con la cookie indicando que el usuario ha votado
    response = make_response(jsonify({"mensaje": "Voto registrado con éxito."}))
    response.set_cookie('has_voted', 'true', max_age=60*60*24)  # Cookie válida por 1 día
    return response

@app.route('/resultados', methods=['GET'])
def resultados():
    return jsonify(options)

if __name__ == '__main__':
    app.run(debug=True)