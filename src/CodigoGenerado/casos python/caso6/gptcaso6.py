from flask import Flask, request, redirect, render_template_string, make_response

app = Flask(__name__)

# Opciones de votación
opciones = ["Python", "JavaScript", "C", "Rust"]
votos = {opcion: 0 for opcion in opciones}

TEMPLATE = """
<!DOCTYPE html>
<html>
<head><title>Votación</title></head>
<body>
    <h1>Vota por tu lenguaje favorito</h1>
    {% if ya_votado %}
        <p>Ya has votado. ¡Gracias!</p>
        <h2>Resultados:</h2>
        <ul>
            {% for opcion, cantidad in votos.items() %}
                <li>{{ opcion }}: {{ cantidad }} votos</li>
            {% endfor %}
        </ul>
    {% else %}
        <form method="POST">
            {% for opcion in opciones %}
                <input type="radio" name="voto" value="{{ opcion }}" required> {{ opcion }}<br>
            {% endfor %}
            <input type="submit" value="Votar">
        </form>
    {% endif %}
</body>
</html>
"""

@app.route('/', methods=['GET', 'POST'])
def votar():
    ya_votado = request.cookies.get('ya_votado')
    
    if request.method == 'POST' and not ya_votado:
        voto = request.form.get('voto')
        if voto in votos:
            votos[voto] += 1
        resp = make_response(redirect('/'))
        resp.set_cookie('ya_votado', '1', max_age=60*60*24*365)  # 1 año
        return resp
    
    return render_template_string(TEMPLATE, votos=votos, opciones=opciones, ya_votado=ya_votado)

if __name__ == '__main__':
    app.run(debug=True)
