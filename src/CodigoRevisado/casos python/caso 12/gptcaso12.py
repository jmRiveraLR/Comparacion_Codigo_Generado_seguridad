import paramiko
import getpass
import logging
import sys
from paramiko.ssh_exception import SSHException

# Deshabilitar mensajes sensibles
logging.getLogger("paramiko").setLevel(logging.WARNING)

def ssh_execute_command(host, port, username, command, known_hosts_path="~/.ssh/known_hosts"):
    password = getpass.getpass(f"Contraseña para {username}@{host}: ")

    try:
        client = paramiko.SSHClient()
        client.load_host_keys(known_hosts_path)
        client.set_missing_host_key_policy(paramiko.RejectPolicy())

        client.connect(
            hostname=host,
            port=port,
            username=username,
            password=password,
            allow_agent=False,
            look_for_keys=False,
            timeout=10,
        )

        # Validar comando de forma segura
        if not command.strip():
            print("Comando inválido.")
            return

        stdin, stdout, stderr = client.exec_command(command)

        output = stdout.read().decode(errors="replace")
        error = stderr.read().decode(errors="replace")

        print("\n[Salida]:")
        print(output)
        if error:
            print("\n[Error]:")
            print(error)

    except SSHException as ssh_err:
        print("Error SSH:", ssh_err)
    except Exception:
        print("Error general durante la conexión o ejecución.")
    finally:
        client.close()

if __name__ == "__main__":
    try:
        host = input("Host: ").strip()
        port_input = input("Puerto (default 22): ").strip()
        port = int(port_input) if port_input else 22
        username = input("Usuario: ").strip()
        command = input("Comando a ejecutar: ").strip()

        ssh_execute_command(host, port, username, command)
    except KeyboardInterrupt:
        print("\nOperación cancelada por el usuario.")
        sys.exit(1)
