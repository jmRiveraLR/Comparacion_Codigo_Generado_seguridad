import os
import time
import uuid
from datetime import datetime, timedelta
from enum import Enum
from functools import wraps

from flask import Flask, request, jsonify
from redis import Redis
from rq import Queue, Worker
from rq.job import Job
from sqlalchemy import create_engine, Column, String, Integer, DateTime, Enum as SQLEnum
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

# Configuración inicial
app = Flask(__name__)
Base = declarative_base()

# Enums para prioridades y estados
class Priority(Enum):
    CRITICAL = 'critical'
    HIGH = 'high'
    NORMAL = 'normal'

class TaskStatusEnum(Enum):
    QUEUED = 'queued'
    STARTED = 'started'
    FINISHED = 'finished'
    FAILED = 'failed'

# Modelo de base de datos
class TaskStatus(Base):
    __tablename__ = 'task_status'
    
    id = Column(String(36), primary_key=True)
    task_name = Column(String(100))
    priority = Column(SQLEnum(Priority))
    status = Column(SQLEnum(TaskStatusEnum))
    created_at = Column(DateTime)
    started_at = Column(DateTime, nullable=True)
    finished_at = Column(DateTime, nullable=True)
    result = Column(String, nullable=True)
    error = Column(String, nullable=True)

# Configuración de conexiones
DATABASE_URL = os.getenv("DATABASE_URL", "postgresql://user:password@localhost/taskdb")
engine = create_engine(DATABASE_URL)
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

REDIS_URL = os.getenv("REDIS_URL", "redis://localhost:6379/0")
redis_conn = Redis.from_url(REDIS_URL)

# Crear tablas si no existen
Base.metadata.create_all(bind=engine)

# Configuración de colas
critical_queue = Queue('critical', connection=redis_conn, default_timeout=10)
high_queue = Queue('high', connection=redis_conn)
normal_queue = Queue('normal', connection=redis_conn)

def get_queue_by_priority(priority):
    return {
        Priority.CRITICAL: critical_queue,
        Priority.HIGH: high_queue,
        Priority.NORMAL: normal_queue
    }.get(priority, normal_queue)

# Registro de tareas disponibles
TASK_REGISTRY = {}

def register_task(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        job = Job.fetch(Job.get_current_job_id(), connection=redis_conn)
        db = SessionLocal()
        try:
            # Actualizar estado a "started"
            task = db.query(TaskStatus).filter(TaskStatus.id == job.id).first()
            if task:
                task.status = TaskStatusEnum.STARTED
                task.started_at = datetime.utcnow()
                db.commit()
            
            # Ejecutar tarea
            result = func(*args, **kwargs)
            
            # Actualizar estado a "finished"
            if task:
                task.status = TaskStatusEnum.FINISHED
                task.finished_at = datetime.utcnow()
                task.result = str(result)
                db.commit()
            
            return result
        except Exception as e:
            # Actualizar estado a "failed"
            if task:
                task.status = TaskStatusEnum.FAILED
                task.finished_at = datetime.utcnow()
                task.error = str(e)
                db.commit()
            raise
        finally:
            db.close()
    
    TASK_REGISTRY[func.__name__] = wrapper
    return wrapper

# Ejemplo de tareas
@register_task
def critical_task(data):
    """Tarea crítica que debe completarse en <10 segundos"""
    time.sleep(1)  # Simular trabajo
    return f"CRITICAL: Processed {data}"

@register_task
def high_priority_task(data):
    time.sleep(3)
    return f"HIGH: Processed {data}"

@register_task
def normal_task(data):
    time.sleep(5)
    return f"NORMAL: Processed {data}"

# API Flask
@app.route('/task', methods=['POST'])
def enqueue_task():
    data = request.json
    task_name = data.get('task_name')
    priority_name = data.get('priority', 'normal')
    args = data.get('args', [])
    kwargs = data.get('kwargs', {})
    
    try:
        priority = Priority(priority_name)
    except ValueError:
        return jsonify({"error": "Invalid priority"}), 400
    
    if task_name not in TASK_REGISTRY:
        return jsonify({"error": "Task not found"}), 404
    
    # Encolar tarea
    queue = get_queue_by_priority(priority)
    job_id = str(uuid.uuid4())
    job = queue.enqueue(
        TASK_REGISTRY[task_name],
        *args,
        **kwargs,
        job_id=job_id,
        meta={'priority': priority.value}
    )
    
    # Registrar en base de datos
    db = SessionLocal()
    try:
        task = TaskStatus(
            id=job_id,
            task_name=task_name,
            priority=priority,
            status=TaskStatusEnum.QUEUED,
            created_at=datetime.utcnow()
        )
        db.add(task)
        db.commit()
        return jsonify({
            "job_id": job_id,
            "status": "queued",
            "priority": priority.value
        }), 202
    except Exception as e:
        db.rollback()
        return jsonify({"error": str(e)}), 500
    finally:
        db.close()

@app.route('/task/<job_id>', methods=['GET'])
def get_task_status(job_id):
    db = SessionLocal()
    try:
        task = db.query(TaskStatus).filter(TaskStatus.id == job_id).first()
        if not task:
            return jsonify({"error": "Task not found"}), 404
        
        return jsonify({
            "job_id": task.id,
            "task_name": task.task_name,
            "priority": task.priority.value,
            "status": task.status.value,
            "result": task.result,
            "error": task.error,
            "created_at": task.created_at.isoformat() if task.created_at else None,
            "started_at": task.started_at.isoformat() if task.started_at else None,
            "finished_at": task.finished_at.isoformat() if task.finished_at else None
        })
    finally:
        db.close()

# Worker personalizado
class PriorityWorker(Worker):
    def __init__(self, queues, *args, **kwargs):
        super().__init__(queues, *args, **kwargs)
    
    def handle_job_failure(self, job, *args, **kwargs):
        db = SessionLocal()
        try:
            task = db.query(TaskStatus).filter(TaskStatus.id == job.id).first()
            if task:
                task.status = TaskStatusEnum.FAILED
                task.finished_at = datetime.utcnow()
                task.error = str(job.exc_info)
                db.commit()
        finally:
            db.close()
        return super().handle_job_failure(job, *args, **kwargs)

# Función para iniciar worker
def start_worker(queue_name):
    queue_map = {
        'critical': critical_queue,
        'high': high_queue,
        'normal': normal_queue
    }
    
    if queue_name not in queue_map:
        raise ValueError(f"Invalid queue name: {queue_name}")
    
    worker = PriorityWorker([queue_map[queue_name]])
    worker.work()

# Función de recuperación
def recover_stuck_tasks():
    db = SessionLocal()
    try:
        threshold = datetime.utcnow() - timedelta(minutes=5)
        stuck_tasks = db.query(TaskStatus).filter(
            TaskStatus.status == TaskStatusEnum.STARTED,
            TaskStatus.started_at < threshold
        ).all()
        
        for task in stuck_tasks:
            print(f"Re-enqueuing stuck task: {task.id}")
            queue = get_queue_by_priority(Priority(task.priority))
            queue.enqueue(
                TASK_REGISTRY[task.task_name],
                job_id=task.id,
                meta={'priority': task.priority, 'retry': True}
            )
            task.status = TaskStatusEnum.QUEUED
            task.started_at = None
            db.commit()
    finally:
        db.close()

if __name__ == '__main__':
    import argparse
    
    parser = argparse.ArgumentParser(description='Task Service')
    subparsers = parser.add_subparsers(dest='command')
    
    # Parser para API
    api_parser = subparsers.add_parser('api')
    api_parser.add_argument('--port', type=int, default=5000)
    
    # Parser para worker
    worker_parser = subparsers.add_parser('worker')
    worker_parser.add_argument('queue', choices=['critical', 'high', 'normal'])
    
    # Parser para recovery
    recovery_parser = subparsers.add_parser('recover')
    
    args = parser.parse_args()
    
    if args.command == 'api':
        app.run(host='0.0.0.0', port=args.port)
    elif args.command == 'worker':
        start_worker(args.queue)
    elif args.command == 'recover':
        recover_stuck_tasks()
    else:
        parser.print_help()