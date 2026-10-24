from src.hardware.stm32_hardware import STM32Hardware
import time

stm32 = STM32Hardware("COM5")

try:
    stm32.connect()
    print("Connexion : OK")

    stm32.adc_select(0)

    print("Acquisition en cours...")
    start = time.perf_counter()

    samples = stm32.adc_capture()

    elapsed = time.perf_counter() - start

    print(f"Durée totale : {elapsed:.2f} secondes")

    print("Nombre d'échantillons :", len(samples))
    print("Premier échantillon :", samples[0])
    print("Dernier échantillon :", samples[-1])

    if len(samples) > 1:
        duration = samples[-1][0] - samples[0][0]
        frequency = (len(samples) - 1) * 1000 / duration

        print(f"Durée réelle : {duration} ms")
        print(f"Fréquence d'échantillonnage moyenne : {frequency:.1f} Hz")

finally:
    stm32.disconnect()
    print("Déconnexion : OK")