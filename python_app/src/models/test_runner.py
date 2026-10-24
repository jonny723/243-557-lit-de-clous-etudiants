

class TestRunner:
    """Exécute les séquences de tests avec le STM32."""

    def __init__(self, stm32):
        self.stm32 = stm32

    def test_voltage(self, adc_channel: int = 0) -> float:
        """Mesure la tension sur une entrée ADC."""

        #  Sélectionner le canal ADC

        #  Vérifier que la sélection a réussi

        #  Lire la tension

        #  Retourner la tension mesurée
        
        pass

    def test_frequency(
        self,
        pwm_channel: int = 0,
        adc_channel: int = 0,
        frequency: int = 10,
        duty: int = 50
    ) -> dict:
        """Génère un PWM, acquiert le signal et mesure ses caractéristiques."""

        # Configuration du PWM

        # Sélectionner le canal PWM

        # Configurer la fréquence du PWM

        # Configurer le rapport cyclique du PWM

        # Vérifier les réponses du STM32


        # Configuration de l'ADC

        # Sélectionner le canal ADC

        # Vérifier la réponse du STM32


        # Acquisition
        samples = self.stm32.adc_capture()

        if not samples:
            raise RuntimeError("Aucun échantillon ADC reçu")


        # Traitement des échantillons

        # Convertir les temps en secondes et les valeurs ADC en tensions

        # Déterminer les tensions minimale et maximale

        # Déterminer un seuil pour détecter les transitions

        # Détecter les fronts montants et descendants

        # Calculer la période moyenne du signal

        # Calculer la fréquence mesurée

        # Calculer le rapport cyclique mesuré

        return {
            "frequency_requested": frequency,
            "frequency_measured": measured_frequency,
            "duty_requested": duty,
            "duty_measured": measured_duty,
            "voltage_min": v_min,
            "voltage_max": v_max,
            "samples": samples,
        }

    def test_frequency_response(
        self,
        frequency: int,
        pwm_channel: int = 0,
        adc_channel: int = 0
    ) -> dict:
        """Acquiert la réponse du circuit à une fréquence donnée."""

        if not 1 <= frequency <= 100:
            raise ValueError(
                "La fréquence doit être comprise entre 1 et 100 Hz"
            )

        # Configuration du générateur PWM

        # Sélectionner le canal PWM

        # Configurer la fréquence demandée

        # Configurer le rapport cyclique à 50 %

        # Vérifier les réponses du STM32


        # Configuration de l'ADC

        # Sélectionner le canal ADC

        # Vérifier la réponse du STM32


        # Acquisition de 3 secondes
        samples = self.stm32.adc_capture()

        if not samples:
            raise RuntimeError("Aucun échantillon ADC reçu")


        # Conversion des échantillons

        # Convertir les temps d'acquisition en secondes

        # Déterminer le facteur de conversion selon
        # la résolution de l'ADC (12, 14, 16 ou 18 bits)

        # Convertir les valeurs ADC en tensions


        # Préparation des résultats
        return {
            "frequency": frequency,
            "time": time_s,
            "voltage": voltage,
            "samples": samples,
        }