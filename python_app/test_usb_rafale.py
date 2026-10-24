import serial
from src.config import STM32_PORT, STM32_BAUDRATE, STM32_TIMEOUT

with serial.Serial(
    STM32_PORT,
    baudrate=STM32_BAUDRATE,
    timeout=STM32_TIMEOUT
) as stm32:

    nombre = 20

    # Envoyer 20 commandes sans attendre
    for i in range(nombre):
        stm32.write(b"PING\n")

    # Lire les réponses
    reponses = []

    for i in range(nombre):
        reponse = stm32.readline().decode("utf-8").strip()

        if not reponse:
            break

        reponses.append(reponse)

    print(f"Commandes envoyées : {nombre}")
    print(f"Réponses reçues : {len(reponses)}")
    print(f"Réponses PONG : {reponses.count('PONG')}")