import time
from rq import Queue, Worker, Connection
from redis import Redis
import psycopg2

# Configuración de Redis
redis_conn = Redis()

# Configuración de PostgreSQL
def create_postgresql_connection():
    return psycopg2.connect(
        dbname="nombre_base_datos",
        user="usuario",
        password="contraseña",
        host="localhost",
        port="5432"
    )

# Función para guardar el estado de una tarea en PostgreSQL
def save_task_state_to_postgresql(task_id, state):
    conn = create_postgresql_connection()
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO task_states (task_id, state, timestamp)
        VALUES (%s, %s, NOW())
        ON CONFLICT (task_id) DO UPDATE SET state = EXCLUDED.state;
    """, (task_id, state))
    conn.commit()
    cursor.close()
    conn.close()

# Función para procesar tareas
def process_task(task_id, critical=False):
    save_task_state_to_postgresql(task_id, 'en_proceso')
    time.sleep(5 if critical else 15)  # Simula tiempo de procesamiento
    save_task_state_to_postgresql(task_id, 'completada')
    return f"Tarea {task_id} completada"

# Creación de colas con prioridades
critical_queue = Queue('critical', connection=redis_conn)
high_queue = Queue('high', connection=redis_conn)
low_queue = Queue('low', connection=redis_conn)

# Ejemplo de encolar tareas
critical_queue.enqueue(process_task, args=("task_1", True))
high_queue.enqueue(process_task, args=("task_2", False))
low_queue.enqueue(process_task, args=("task_3", False))

# Configuración de trabajadores
with Connection(redis_conn):
    workers = [
        Worker([critical_queue], name='CriticalWorker'),
        Worker([high_queue], name='HighPriorityWorker'),
        Worker([low_queue], name='LowPriorityWorker'),
    ]

    for worker in workers:
        worker.work()