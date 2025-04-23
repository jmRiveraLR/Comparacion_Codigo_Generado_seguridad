import sys
import time
import datetime
from redis import Redis
from rq import Queue, Worker, Connection, get_current_job
from sqlalchemy import create_engine, Column, Integer, String, DateTime
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

# ------------------------------------------
# Configuración de Redis y PostgreSQL
# ------------------------------------------
redis_conn = Redis()
DATABASE_URL = "postgresql://usuario:password@localhost:5432/rq_tasks"

# Conexión SQLAlchemy
engine = create_engine(DATABASE_URL)
SessionLocal = sessionmaker(bind=engine)
Base = declarative_base()

# ------------------------------------------
# Modelo de estado de tareas
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
# Función de tarea
# ------------------------------------------
def process_task(duration, critical=False):
    job = get_current_job()
    db = SessionLocal()
    try:
        status = db.query(TaskStatus).filter_by(job_id=job.id).first()
        if status:
            status.status = 'started'
            db.commit()

        if critical:
            assert duration <= 10, "Tarea crítica excede el tiempo límite"

        time.sleep(duration)

        if status:
            status.status = 'finished'
            status.completed_at = datetime.datetime.utcnow()
            db.commit()
    except Exception as e:
        if status:
            status.status = f'failed: {str(e)}'
            db.commit()
    finally:
        db.close()

# ------------------------------------------
# Encolar tareas
# ------------------------------------------
def enqueue_task(priority, duration, critical=False):
    queues = {
        'high': Queue('high', connection=redis_conn),
        'medium': Queue('medium', connection=redis_conn),
        'low': Queue('low', connection=redis_conn),
    }
    q = queues.get(priority)
    if not q:
        print("Prioridad inválida: usa high, medium o low")
        return

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
        print("Prioridad inválida: usa high, medium o low")
        return
    with Connection(redis_conn):
        print(f"🔧 Iniciando worker para prioridad '{priority}'")
        Worker([Queue(priority)]).work()

# ------------------------------------------
# Menú CLI
# ------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Modo de uso:\n"
              "  python tareas_servicio.py enqueue <prioridad> <duración> <True|False>\n"
              "  python tareas_servicio.py worker <prioridad>")
        sys.exit(1)

    modo = sys.argv[1]

    if modo == "enqueue":
        if len(sys.argv) != 5:
            print("Uso: python tareas_servicio.py enqueue <prioridad> <duración> <True|False>")
            sys.exit(1)
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

## crear worker y encolar tareas: 
## python tareas_servicio.py worker high
## python tareas_servicio.py enqueue high 5 True
## python tareas_servicio.py enqueue low 20 False