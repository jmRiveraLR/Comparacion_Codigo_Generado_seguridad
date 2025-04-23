import sys
import time
import datetime
import os
from redis import Redis
from rq import Queue, Worker, Connection, get_current_job
from sqlalchemy import create_engine, Column, Integer, String, DateTime
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

# ------------------------------------------
# Configuración segura usando variables de entorno
# ------------------------------------------
DATABASE_URL = os.getenv("DATABASE_URL", "postgresql://usuario:password@localhost:5432/rq_tasks")
AUTH_TOKEN = os.getenv("AUTH_TOKEN", "default_token")

redis_conn = Redis()

# SQLAlchemy
engine = create_engine(DATABASE_URL, client_encoding='utf8')
SessionLocal = sessionmaker(bind=engine)
Base = declarative_base()

# ------------------------------------------
# Modelo seguro
# ------------------------------------------
class TaskStatus(Base):
    __tablename__ = "task_status"
    id = Column(Integer, primary_key=True, index=True)
    job_id = Column(String, unique=True, index=True)
    status = Column(String)
    enqueued_at = Column(DateTime, default=datetime.datetime.utcnow)
    completed_at = Column(DateTime, nullable=True)

Base.metadata.create_all(bind=engine)

# ------------------------------------------
# Tarea con control de errores
# ------------------------------------------
def process_task(duration, critical=False):
    job = get_current_job()
    db = SessionLocal()
    try:
        status = db.query(TaskStatus).filter_by(job_id=job.id).first()
        if status:
            status.status = 'started'
            db.commit()

        # Prevención CWE-400
        if duration > 60:
            raise ValueError("Duración máxima permitida es 60 segundos.")
        if critical:
            assert duration <= 10, "Tarea crítica excede el límite"

        time.sleep(duration)

        if status:
            status.status = 'finished'
            status.completed_at = datetime.datetime.utcnow()
            db.commit()
    except Exception as e:
        if status:
            status.status = 'failed'
            db.commit()
    finally:
        db.close()

# ------------------------------------------
# Encolar tareas con autenticación y validación
# ------------------------------------------
def enqueue_task(priority, duration, critical=False):
    if not isinstance(duration, int) or duration < 0:
        print("Duración inválida.")
        return
    if priority not in ['high', 'medium', 'low']:
        print("Prioridad inválida.")
        return

    queues = {
        'high': Queue('high', connection=redis_conn),
        'medium': Queue('medium', connection=redis_conn),
        'low': Queue('low', connection=redis_conn),
    }

    q = queues[priority]
    job = q.enqueue(process_task, duration, critical)
    db = SessionLocal()
    status = TaskStatus(job_id=job.id, status='queued')
    db.add(status)
    db.commit()
    db.close()
    print(f"[✔] Tarea {job.id} encolada con prioridad '{priority}'")

# ------------------------------------------
# Worker por prioridad
# ------------------------------------------
def run_worker(priority):
    if priority not in ['high', 'medium', 'low']:
        print("Prioridad inválida.")
        return
    with Connection(redis_conn):
        print(f"🔧 Iniciando worker para prioridad '{priority}'")
        Worker([Queue(priority)]).work()

# ------------------------------------------
# Autenticación simple por token (mejora CWE-306)
# ------------------------------------------
def check_auth():
    token = os.getenv("RQ_AUTH", "")
    if token != AUTH_TOKEN:
        print("❌ Autenticación fallida.")
        sys.exit(1)

# ------------------------------------------
# CLI protegido
# ------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso:\n"
              "  python tareas_servicio.py enqueue <prioridad> <duración> <True|False>\n"
              "  python tareas_servicio.py worker <prioridad>")
        sys.exit(1)

    modo = sys.argv[1]

    if modo == "enqueue":
        if len(sys.argv) != 5:
            print("Uso: python tareas_servicio.py enqueue <prioridad> <duración> <True|False>")
            sys.exit(1)
        check_auth()
        prioridad = sys.argv[2]
        duracion = int(sys.argv[3])
        critico = sys.argv[4].lower() == 'true'
        enqueue_task(prioridad, duracion, critico)

    elif modo == "worker":
        if len(sys.argv) != 3:
            print("Uso: python tareas_servicio.py worker <prioridad>")
            sys.exit(1)
        prioridad = sys.argv[2]
        run_worker(prioridad)

    else:
        print("Comando inválido.")
