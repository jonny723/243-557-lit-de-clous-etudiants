from src.hardware.stm32_hardware import STM32Hardware
import time

stm32 = STM32Hardware("COM5")

try:
    stm32.connect()
    print("Connexion : OK")

    print("PING :", stm32.ping())
    print("ADC SELECT 0 :", stm32.adc_select(0))

    print("Acquisition de 3 secondes...")

    # Délai de réponse temporairement augmenté
    response = stm32._send_command("ADC CAPTURE 3000", timeout=5.0)
    print("Résultat :", response)

    print("\nTest du transfert par blocs :")

    for index in [0, 4, 8, 12, 16]:
        start = time.perf_counter()

        response = stm32._send_command(f"ADC BLOCK {index}")

        elapsed = time.perf_counter() - start

        print(f"Bloc {index} : {elapsed:.3f} s — {response}")

finally:
    stm32.disconnect()
    print("Déconnexion : OK")