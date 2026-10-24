from src.hardware.stm32_hardware import STM32Hardware

stm32 = STM32Hardware("COM5")

try:
    stm32.connect()
    print("Connexion : OK")

    print("PING :", stm32.ping())

    print("Résolution :", stm32.adc_resolution, "bits")

    stm32.adc_select(0)

    voltage = stm32.adc_read_voltage()
    print(f"Tension : {voltage:.3f} V")

finally:
    stm32.disconnect()
    print("Déconnexion : OK")