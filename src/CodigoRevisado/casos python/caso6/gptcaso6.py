from flask import Flask, request, redirect, render_template_string, make_response
from flask_wtf import FlaskForm, CSRFProtect
from wtforms import RadioField, SubmitField
from wtforms.validators import InputRequired
import os

app = Flask(__name__)
app.secret_key = os.urandom(32)
csrf = CSRFProtect(app)

# Opciones de votación
opciones = ["Python", "JavaScript", "C", "Rust"]
votos = {opcion: 0 for opcion in opciones}

# Formulario protegido con CSRF
class VotoForm(FlaskForm):
    voto = RadioField("Voto", choices=[(op, op) for op in opciones], validators=[InputRequired()])
    submit = SubmitField("Votar")

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
            {{ form.hidden_tag() }}
            {% for subfield in form.voto %}
                <input type="radio" name="{{ form.voto.name }}" value="{{ subfield.data }}" required> {{ subfield.label.text }}<br>
            {% endfor %}
            {{ form.submit() }}
        </form>
    {% endif %}
</body>
</html>
"""

@app.route('/', methods=['GET', 'POST'])
def votar():
    form = VotoForm()
    ya_votado = request.cookies.get('ya_votado')
    
    if request.method == 'POST' and form.validate_on_submit() and not ya_votado:
        voto = form.voto.data
        if voto in votos:
            votos[voto] += 1
        resp = make_response(redirect('/'))
        # Corrección de cookies inseguras
        resp.set_cookie('ya_votado', '1', max_age=60*60*24*365, secure=True, httponly=True, samesite='Lax')
        return resp

    return render_template_string(TEMPLATE, votos=votos, form=form, ya_votado=ya_votado)

if __name__ == '__main__':
    app.run(debug=False)  # Desactivar debug en producción
