import os
import time
import uuid
import logging
from datetime import datetime, timedelta
from enum import Enum
from functools import wraps
import bleach  # Para sanitización
from werkzeug.middleware.proxy_fix import ProxyFix
from flask import Flask, request, jsonify
from flask_limiter import Limiter
from flask_limiter.util import get_remote_address
from redis import Redis
from rq import Queue, Worker
from rq.job import Job
from sqlalchemy import create_engine, Column, String, DateTime, Enum as SQLEnum, text
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
import jwt
from jwt.exceptions import InvalidTokenError
from dotenv import load_dotenv

# Cargar variables de entorno desde .env
load_dotenv()

# Configuración de logging de seguridad
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('security.log'),
        logging.StreamHandler()
    ]
)
security_logger = logging.getLogger('security')

app = Flask(__name__)
app.wsgi_app = ProxyFix(app.wsgi_app, x_for=1, x_proto=1, x_host=1, x_prefix=1)

# Configuración de rate limiting
limiter = Limiter(
    app=app,
    key_func=get_remote_address,
    default_limits=["200 per day", "50 per hour"]
)

# Configuración de seguridad
app.config['SECRET_KEY'] = os.getenv('SECRET_KEY', os.urandom(32))
app.config['JWT_ALGORITHM'] = 'HS256'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB

Base = declarative_base()

# Enums
class Priority(Enum):
    CRITICAL = 'critical'
    HIGH = 'high'
    NORMAL = 'normal'

class TaskStatusEnum(Enum):
    QUEUED = 'queued'
    STARTED = 'started'
    FINISHED = 'finished'
    FAILED = 'failed'

# Modelo de base de datos con índices y constraints
class TaskStatus(Base):
    __tablename__ = 'task_status'
    
    id = Column(String(36), primary_key=True)
    task_name = Column(String(100), nullable=False)
    priority = Column(SQLEnum(Priority), nullable=False)
    status = Column(SQLEnum(TaskStatusEnum), nullable=False)
    created_at = Column(DateTime, nullable=False)
    started_at = Column(DateTime)
    finished_at = Column(DateTime)
    result = Column(String)
    error = Column(String)

# Configuración segura de conexiones
DATABASE_URL = os.getenv("DATABASE_URL")
if not DATABASE_URL:
    raise ValueError("DATABASE_URL no está configurado")
engine = create_engine(DATABASE_URL, pool_pre_ping=True)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

REDIS_URL = os.getenv("REDIS_URL")
if not REDIS_URL:
    raise ValueError("REDIS_URL no está configurado")
redis_conn = Redis.from_url(REDIS_URL, ssl_cert_reqs=None)

# Crear tablas
Base.metadata.create_all(bind=engine)

# Configuración segura de colas
critical_queue = Queue('critical', connection=redis_conn, default_timeout=10)
high_queue = Queue('high', connection=redis_conn)
normal_queue = Queue('normal', connection=redis_conn)

def get_queue_by_priority(priority):
    return {
        Priority.CRITICAL: critical_queue,
        Priority.HIGH: high_queue,
        Priority.NORMAL: normal_queue
    }.get(priority, normal_queue)

# Registro de tareas con sanitización
TASK_REGISTRY = {}

def validate_input(data, max_length=100):
    """Valida y sanitiza la entrada"""
    if not data or len(data) > max_length:
        raise ValueError(f"Input inválido. Longitud máxima: {max_length}")
    return bleach.clean(data)

def register_task(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        job = Job.fetch(Job.get_current_job_id(), connection=redis_conn)
        db = SessionLocal()
        try:
            # Validar y sanitizar argumentos
            clean_args = [validate_input(str(arg)) if isinstance(arg, (str, bytes)) else arg 
                         for arg in args]
            clean_kwargs = {k: validate_input(str(v)) if isinstance(v, (str, bytes)) else v 
                           for k, v in kwargs.items()}
            
            # Actualizar estado usando parámetros preparados
            db.execute(
                text("UPDATE task_status SET status=:status, started_at=:started WHERE id=:id"),
                {"status": TaskStatusEnum.STARTED.value, "started": datetime.utcnow(), "id": job.id}
            )
            db.commit()
            
            # Ejecutar tarea
            result = func(*clean_args, **clean_kwargs)
            
            # Actualizar estado
            db.execute(
                text("UPDATE task_status SET status=:status, finished_at=:finished, result=:result WHERE id=:id"),
                {
                    "status": TaskStatusEnum.FINISHED.value,
                    "finished": datetime.utcnow(),
                    "result": str(result),
                    "id": job.id
                }
            )
            db.commit()
            
            return result
        except Exception as e:
            db.rollback()
            security_logger.error(f"Error en tarea {job.id}: {str(e)}")
            raise
        finally:
            db.close()
    
    TASK_REGISTRY[func.__name__] = wrapper
    return wrapper

# Autenticación JWT
def token_required(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        token = request.headers.get('Authorization')
        if not token:
            security_logger.warning("Intento de acceso no autorizado")
            return jsonify({"error": "Token faltante"}), 401
        
        try:
            token = token.split()[1]  # Bearer <token>
            data = jwt.decode(token, app.config['SECRET_KEY'], algorithms=[app.config['JWT_ALGORITHM']])
        except InvalidTokenError as e:
            security_logger.warning(f"Token inválido: {str(e)}")
            return jsonify({"error": "Token inválido"}), 401
        
        return f(*args, **kwargs)
    return decorated

# Tareas de ejemplo con validación
@register_task
def critical_task(data):
    """Tarea crítica con validación de entrada"""
    if not data or len(data) > 100:
        raise ValueError("Datos inválidos")
    time.sleep(1)
    return f"CRITICAL: Procesado {bleach.clean(data)}"

# API segura
@app.route('/task', methods=['POST'])
@token_required
@limiter.limit("10/minute")
def enqueue_task():
    try:
        data = request.get_json(force=True, silent=True) or {}
        
        # Validación estricta de entrada
        task_name = validate_input(data.get('task_name', ''), 50)
        priority_name = validate_input(data.get('priority', 'normal'), 10)
        args = data.get('args', [])
        kwargs = data.get('kwargs', {})
        
        try:
            priority = Priority(priority_name)
        except ValueError:
            security_logger.warning(f"Prioridad inválida: {priority_name}")
            return jsonify({"error": "Prioridad inválida"}), 400
        
        if task_name not in TASK_REGISTRY:
            security_logger.warning(f"Tarea no encontrada: {task_name}")
            return jsonify({"error": "Tarea no encontrada"}), 404
        
        # Validar argumentos
        if not isinstance(args, list) or not isinstance(kwargs, dict):
            security_logger.warning("Argumentos inválidos")
            return jsonify({"error": "Argumentos inválidos"}), 400
        
        # Encolar tarea
        job_id = str(uuid.uuid4())
        queue = get_queue_by_priority(priority)
        job = queue.enqueue(
            TASK_REGISTRY[task_name],
            *args,
            **kwargs,
            job_id=job_id,
            meta={'priority': priority.value}
        )
        
        # Registrar en base de datos con parámetros preparados
        db = SessionLocal()
        try:
            db.execute(
                text("""
                    INSERT INTO task_status 
                    (id, task_name, priority, status, created_at)
                    VALUES (:id, :task, :priority, :status, :created)
                    """),
                {
                    "id": job_id,
                    "task": task_name,
                    "priority": priority.value,
                    "status": TaskStatusEnum.QUEUED.value,
                    "created": datetime.utcnow()
                }
            )
            db.commit()
            return jsonify({
                "job_id": job_id,
                "status": "queued",
                "priority": priority.value
            }), 202
        except Exception as e:
            db.rollback()
            security_logger.error(f"Error en base de datos: {str(e)}")
            return jsonify({"error": "Error interno"}), 500
        finally:
            db.close()
            
    except Exception as e:
        security_logger.error(f"Error en enqueue_task: {str(e)}")
        return jsonify({"error": "Solicitud inválida"}), 400

@app.route('/task/<job_id>', methods=['GET'])
@token_required
def get_task_status(job_id):
    try:
        # Validar job_id
        if not job_id or len(job_id) > 36:
            return jsonify({"error": "ID inválido"}), 400
            
        db = SessionLocal()
        try:
            # Consulta segura con parámetros preparados
            result = db.execute(
                text("""
                    SELECT id, task_name, priority, status, result, error,
                           created_at, started_at, finished_at
                    FROM task_status
                    WHERE id = :id
                    """),
                {"id": job_id}
            ).fetchone()
            
            if not result:
                return jsonify({"error": "Tarea no encontrada"}), 404
            
            return jsonify({
                "job_id": result[0],
                "task_name": result[1],
                "priority": result[2],
                "status": result[3],
                "result": result[4],
                "error": result[5],
                "created_at": result[6].isoformat() if result[6] else None,
                "started_at": result[7].isoformat() if result[7] else None,
                "finished_at": result[8].isoformat() if result[8] else None
            })
        finally:
            db.close()
    except Exception as e:
        security_logger.error(f"Error en get_task_status: {str(e)}")
        return jsonify({"error": "Error interno"}), 500

class PriorityWorker(Worker):
    def handle_job_failure(self, job, *args, **kwargs):
        db = SessionLocal()
        try:
            db.execute(
                text("""
                    UPDATE task_status 
                    SET status=:status, finished_at=:finished, error=:error
                    WHERE id=:id
                    """),
                {
                    "status": TaskStatusEnum.FAILED.value,
                    "finished": datetime.utcnow(),
                    "error": str(job.exc_info),
                    "id": job.id
                }
            )
            db.commit()
        except Exception as e:
            db.rollback()
            security_logger.error(f"Error al registrar fallo: {str(e)}")
        finally:
            db.close()
        return super().handle_job_failure(job, *args, **kwargs)

def start_worker(queue_name):
    queue_map = {
        'critical': critical_queue,
        'high': high_queue,
        'normal': normal_queue
    }
    
    if queue_name not in queue_map:
        raise ValueError(f"Cola inválida: {queue_name}")
    
    worker = PriorityWorker([queue_map[queue_name]])
    worker.work()

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='Servicio de Tareas Seguro')
    subparsers = parser.add_subparsers(dest='command', required=True)
    
    api_parser = subparsers.add_parser('api')
    api_parser.add_argument('--port', type=int, default=5000)
    
    worker_parser = subparsers.add_parser('worker')
    worker_parser.add_argument('queue', choices=['critical', 'high', 'normal'])
    
    args = parser.parse_args()
    
    if args.command == 'api':
        app.run(host='0.0.0.0', port=args.port, ssl_context='adhoc')
    elif args.command == 'worker':
        start_worker(args.queue)