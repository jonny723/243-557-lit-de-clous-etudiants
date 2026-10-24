from src.hardware.stm32_hardware import STM32Hardware
from src.config import STM32_PORT

stm32 = STM32Hardware(port=STM32_PORT)

try:
    stm32.connect()

    for i in range(100):
        reponse = stm32.ping()

        if reponse != "PONG":
            print(f"Erreur à la commande {i + 1} : {reponse}")
            break
    else:
        print("100 commandes PING réussies.")

finally:
    stm32.disconnect()