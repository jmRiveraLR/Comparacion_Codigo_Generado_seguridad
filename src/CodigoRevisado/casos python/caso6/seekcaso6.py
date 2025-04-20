import os
from flask import Flask, render_template, request, make_response, redirect, url_for
from flask_wtf.csrf import CSRFProtect
from wtforms import RadioField, SubmitField
from wtforms.validators import DataRequired
from werkzeug.middleware.proxy_fix import ProxyFix

app = Flask(__name__)
app.config['SECRET_KEY'] = os.environ.get('FLASK_SECRET_KEY', os.urandom(24).hex())

# Configuración de seguridad
app.config['SESSION_COOKIE_SECURE'] = True
app.config['SESSION_COOKIE_HTTPONLY'] = True
app.config['SESSION_COOKIE_SAMESITE'] = 'Lax'
app.config['PERMANENT_SESSION_LIFETIME'] = 1800  # 30 minutos

# Ajustes para proxies reversos
app.wsgi_app = ProxyFix(app.wsgi_app, x_for=1, x_proto=1, x_host=1, x_prefix=1)

# Protección CSRF
csrf = CSRFProtect(app)

# Opciones de votación
OPCIONES_VOTACION = {
    'opcion1': 'Opción 1',
    'opcion2': 'Opción 2', 
    'opcion3': 'Opción 3'
}

# Base de datos simulada (en producción usar una real)
from threading import Lock
votos_db = {opcion: 0 for opcion in OPCIONES_VOTACION}
db_lock = Lock()

# Formulario de votación con validación
class VotacionForm(FlaskForm):
    opcion = RadioField(
        'Selecciona una opción:',
        choices=[(k, v) for k, v in OPCIONES_VOTACION.items()],
        validators=[DataRequired(message="Debes seleccionar una opción")]
    )
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
        
        # Bloqueo para evitar condiciones de carrera
        with db_lock:
            votos_db[opcion_seleccionada] += 1
        
        # Configuración segura de cookies
        resp = make_response(redirect(url_for('resultados')))
        resp.set_cookie(
            'ya_voto',
            'si',
            max_age=30*24*60*60,  # 30 días
            secure=True,
            httponly=True,
            samesite='Lax',
            path='/',
            domain=None
        )
        return resp

    return render_template('index.html', form=form, mensaje=mensaje, ya_voto=ya_voto)

@app.route('/resultados')
def resultados():
    if request.cookies.get('ya_voto') != 'si':
        return redirect(url_for('index'))
    
    total_votos = sum(votos_db.values())
    porcentajes = {k: (v / total_votos * 100 if total_votos > 0 else 0) 
                  for k, v in votos_db.items()}
    
    return render_template(
        'resultados.html',
        resultados=votos_db,
        opciones=OPCIONES_VOTACION,
        porcentajes=porcentajes,
        total_votos=total_votos
    )

if __name__ == '__main__':
    # Solo para desarrollo, en producción usar WSGI
    app.run(
        debug=os.environ.get('FLASK_DEBUG', 'false').lower() == 'true',
        host='0.0.0.0',
        port=int(os.environ.get('PORT', 5000))
    )