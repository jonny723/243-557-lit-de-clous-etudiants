import matplotlib.pyplot as plt

from src.hardware.stm32_hardware import STM32Hardware

stm32 = STM32Hardware("COM5")

try:
    stm32.connect()

    # Configuration du PWM
    print("PWM SELECT 0 :", stm32.pwm_select(0))
    print("PWM FREQ 10 :", stm32.pwm_set_frequency(10))
    print("PWM DUTY 50 :", stm32.pwm_set_duty(50))

    # Configuration de l'ADC
    print("ADC SELECT 0 :", stm32.adc_select(0))

    print("Acquisition en cours...")
    samples = stm32.adc_capture()

finally:
    stm32.disconnect()

# Conversion des données
time_s = [timestamp / 1000 for timestamp, raw in samples]
voltage = [raw * 0.001 for timestamp, raw in samples]

# Analyse du signal PWM
v_min = min(voltage)
v_max = max(voltage)
threshold = (v_min + v_max) / 2

# Détection des fronts montants
rising_edges = []

for i in range(1, len(voltage)):
    if voltage[i - 1] < threshold <= voltage[i]:
        rising_edges.append(time_s[i])

# Calcul de la fréquence moyenne
if len(rising_edges) >= 2:
    periods = [
        rising_edges[i] - rising_edges[i - 1]
        for i in range(1, len(rising_edges))
    ]

    average_period = sum(periods) / len(periods)
    measured_frequency = 1 / average_period

    print(f"Fréquence mesurée : {measured_frequency:.2f} Hz")
else:
    print("Fréquence non mesurable : fronts montants insuffisants")

print(f"Tension minimale : {v_min:.3f} V")
print(f"Tension maximale : {v_max:.3f} V")

# Détection des fronts descendants
falling_edges = []

for i in range(1, len(voltage)):
    if voltage[i - 1] >= threshold > voltage[i]:
        falling_edges.append(time_s[i])

# Calcul du rapport cyclique moyen
duty_cycles = []

for start, end in zip(rising_edges, rising_edges[1:]):
    falling = next(
        (t for t in falling_edges if start < t < end),
        None
    )

    if falling is not None:
        duty = (falling - start) / (end - start) * 100
        duty_cycles.append(duty)

if duty_cycles:
    measured_duty = sum(duty_cycles) / len(duty_cycles)
    print(f"Rapport cyclique mesuré : {measured_duty:.1f} %")
else:
    print("Rapport cyclique non mesurable")


# Affichage
plt.plot(time_s, voltage)
plt.xlabel("Temps (s)")
plt.ylabel("Tension ADC (V)")
plt.title("Acquisition ADC - MCP3423")
plt.grid(True)
plt.show()