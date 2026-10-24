from src.hardware.stm32_hardware import STM32Hardware

stm32 = STM32Hardware("COM5")

try:
    stm32.connect()

    print("PING :", stm32.ping())
    print("PWM SELECT 7 :", stm32.pwm_select(7))

    for duty in [0, 10, 25, 50, 63, 75, 90, 100]:
        input(f"\nAppuie sur Entrée pour régler le PWM à {duty} %...")

        response = stm32.pwm_set_duty(duty)
        print(f"PWM DUTY {duty} : {response}")

        input("Mesure la tension sur S7, puis appuie sur Entrée...")

finally:
    stm32.disconnect()