from src.hardware.stm32_hardware import STM32Hardware

stm32 = STM32Hardware("COM5")

try:
    stm32.connect()
    print("Connexion : OK")

    print("PING :", stm32.ping())
    print("PWM SELECT 0 :", stm32.pwm_select(0))

    for frequency in [2, 5, 10, 20, 30, 1000]:
        response = stm32.pwm_set_frequency(frequency)
        print(f"PWM FREQ {frequency} : {response}")

    print("PWM DUTY 50 :", stm32.pwm_set_duty(50))

    try:
        stm32.pwm_set_frequency(0)
    except ValueError as error:
        print("Fréquence invalide :", error)

finally:
    stm32.disconnect()
    print("Déconnexion : OK")