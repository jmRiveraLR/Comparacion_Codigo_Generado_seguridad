import os
import time
from rq import Queue, Worker, Connection
from redis import Redis
import psycopg2
from psycopg2 import sql
from dotenv import load_dotenv

# Cargar variables de entorno
load_dotenv()

# Configuración de Redis
redis_conn = Redis(
    host=os.getenv("REDIS_HOST", "localhost"),
    port=int(os.getenv("REDIS_PORT", 6379)),
    password=os.getenv("REDIS_PASSWORD", None),
    ssl=True  # Uso de SSL para seguridad en las comunicaciones
)

# Configuración de PostgreSQL
def create_postgresql_connection():
    return psycopg2.connect(
        dbname=os.getenv("POSTGRES_DB"),
        user=os.getenv("POSTGRES_USER"),
        password=os.getenv("POSTGRES_PASSWORD"),
        host=os.getenv("POSTGRES_HOST", "localhost"),
        port=os.getenv("POSTGRES_PORT", "5432"),
        sslmode="require"  # Requiere conexión segura
    )

# Función para guardar el estado de una tarea en PostgreSQL
def save_task_state_to_postgresql(task_id, state):
    try:
        conn = create_postgresql_connection()
        cursor = conn.cursor()
        query = sql.SQL("""
            INSERT INTO task_states (task_id, state, timestamp)
            VALUES (%s, %s, NOW())
            ON CONFLICT (task_id) DO UPDATE SET state = EXCLUDED.state;
        """)
        cursor.execute(query, (task_id, state))
        conn.commit()
    except Exception as e:
        print(f"Error al guardar el estado de la tarea: {e}")
    finally:
        if cursor:
            cursor.close()
        if conn:
            conn.close()

# Validación de entrada para task_id
def validate_task_id(task_id):
    if not isinstance(task_id, str) or len(task_id) > 255:
        raise ValueError("Invalid task_id. Must be a string with a maximum length of 255.")

# Función para procesar tareas
def process_task(task_id, critical=False):
    validate_task_id(task_id)
    save_task_state_to_postgresql(task_id, 'en_proceso')
    time.sleep(5 if critical else 15)  # Simula tiempo de procesamiento
    save_task_state_to_postgresql(task_id, 'completada')
    return f"Tarea {task_id} completada"

# Creación de colas con prioridades
critical_queue = Queue('critical', connection=redis_conn)
high_queue = Queue('high', connection=redis_conn)
low_queue = Queue('low', connection=redis_conn)

# Ejemplo de encolar tareas (autenticación requerida)
authenticated_user = os.getenv("AUTH_USER", "default_user")
if authenticated_user == "admin_user":
    critical_queue.enqueue(process_task, args=("task_1", True))
    high_queue.enqueue(process_task, args=("task_2", False))
    low_queue.enqueue(process_task, args=("task_3", False))
else:
    print("Acceso denegado: Usuario no autorizado")

# Configuración de trabajadores
with Connection(redis_conn):
    workers = [
        Worker([critical_queue], name='CriticalWorker'),
        Worker([high_queue], name='HighPriorityWorker'),
        Worker([low_queue], name='LowPriorityWorker'),
    ]

    for worker in workers:
        worker.work()