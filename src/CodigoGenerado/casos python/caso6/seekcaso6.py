from flask import Flask, render_template, request, make_response, redirect, url_for
from flask_wtf.csrf import CSRFProtect
from wtforms import RadioField, SubmitField
from wtforms.validators import DataRequired

app = Flask(__name__)
app.config['SECRET_KEY'] = 'tu_clave_secreta_aqui'  # Cambia esto en producción

# Habilitar CSRF protection
csrf = CSRFProtect(app)

# Opciones de votación
OPCIONES_VOTACION = {
    'opcion1': 'Opción 1',
    'opcion2': 'Opción 2',
    'opcion3': 'Opción 3'
}

# Resultados iniciales
resultados = {opcion: 0 for opcion in OPCIONES_VOTACION.keys()}

# Formulario de votación
class VotacionForm(FlaskForm):
    opcion = RadioField('Selecciona una opción:', 
                       choices=[(k, v) for k, v in OPCIONES_VOTACION.items()],
                       validators=[DataRequired()])
    submit = SubmitField('Votar')

@app.route('/', methods=['GET', 'POST'])
def index():
    form = VotacionForm()
    mensaje = None
    ya_voto = request.cookies.get('ya_voto') == 'si'

    if ya_voto:
        mensaje = "Ya has votado anteriormente. Gracias por participar!"
    elif form.validate_on_submit():
        opcion_seleccionada = form.opcion.data
        resultados[opcion_seleccionada] += 1
        
        # Crear respuesta y establecer cookie
        resp = make_response(redirect(url_for('resultados')))
        resp.set_cookie('ya_voto', 'si', max_age=30*24*60*60)  # Cookie válida por 30 días
        return resp

    return render_template('index.html', form=form, mensaje=mensaje, ya_voto=ya_voto)

@app.route('/resultados')
def resultados():
    ya_voto = request.cookies.get('ya_voto') == 'si'
    if not ya_voto:
        return redirect(url_for('index'))
    
    total_votos = sum(resultados.values())
    porcentajes = {k: (v / total_votos * 100 if total_votos > 0 else 0) 
                  for k, v in resultados.items()}
    
    return render_template('resultados.html', 
                         resultados=resultados,
                         opciones=OPCIONES_VOTACION,
                         porcentajes=porcentajes,
                         total_votos=total_votos)

if __name__ == '__main__':
    app.run(debug=True)