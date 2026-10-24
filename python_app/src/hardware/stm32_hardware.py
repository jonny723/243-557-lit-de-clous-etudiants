
import threading
import serial
from src.config import STM32_BAUDRATE, STM32_TIMEOUT

class STM32Hardware:
    """Gère la communication USB CDC avec le STM32."""

    def __init__(self, port: str):
        self.port = port
        self.serial = None
        self.adc_resolution = 12
        self._lock = threading.RLock()

    def connect(self):
        """Ouvre la connexion USB CDC et configure l'ADC à 12 bits."""
        with self._lock:
            if self.serial is not None and self.serial.is_open:
                return

            self.serial = serial.Serial(
                port=self.port,
                baudrate=STM32_BAUDRATE,
                timeout=STM32_TIMEOUT
            )

            try:
                self.adc_set_resolution(12)
            except Exception:
                self.serial.close()
                self.serial = None
                raise

    def disconnect(self):
        """Ferme la connexion USB CDC de façon sécurisée."""
        with self._lock:
            if self.serial is not None:
                self.serial.close()
                self.serial = None

    def _send_command(self, command: str, timeout: float | None = None) -> str:
        """Envoie une commande au STM32 et retourne sa réponse."""
        with self._lock:
            if self.serial is None or not self.serial.is_open:
                raise RuntimeError("STM32 non connecté")

            previous_timeout = self.serial.timeout

            try:
                if timeout is not None:
                    self.serial.timeout = timeout

                self.serial.write((command + "\n").encode("utf-8"))

                response = self.serial.readline().decode("utf-8").strip()

                if not response:
                    raise TimeoutError(
                        f"Aucune réponse du STM32 : {command}"
                    )

                return response

            finally:
                self.serial.timeout = previous_timeout


    def ping(self) -> str:
        return self._send_command("PING")


    def led_on(self) -> str:
        return self._send_command("LED ON")


    def led_off(self) -> str:
        return self._send_command("LED OFF")

    def pwm_select(self, channel: int) -> str:
        """Sélectionne une sortie du multiplexeur PWM."""
        if not 0 <= channel <= 7:
            raise ValueError("Le canal PWM doit être entre 0 et 7.")

        return self._send_command(f"PWM SELECT {channel}")

    def pwm_set_duty(self, duty: int) -> str:
        """Règle le rapport cyclique du PWM entre 0 et 100 %."""
        if not 0 <= duty <= 100:
            raise ValueError("Le rapport cyclique doit être entre 0 et 100 %.")

        return self._send_command(f"PWM DUTY {duty}")

    def pwm_set_frequency(self, frequency: int) -> str:
        """Configure la fréquence du PWM en hertz."""

        if not 1 <= frequency <= 10000:
            raise ValueError("La fréquence doit être entre 1 et 10000 Hz.")

        return self._send_command(f"PWM FREQ {frequency}")
    

    def gpio_mode(self, board: str, pin: int, mode: str) -> str:
        """Configure une broche GPIO sur la carte ADC ou PWM."""
        board = board.upper()
        mode = mode.upper()

        if board not in ("ADC", "PWM"):
            raise ValueError("La carte doit être ADC ou PWM.")

        if not 0 <= pin <= 7:
            raise ValueError("La broche GPIO doit être entre 0 et 7.")

        if mode not in ("IN", "OUT"):
            raise ValueError("Le mode doit être IN ou OUT.")

        if board == "ADC":
            command = f"GPIO ADC MODE {pin} {mode}"
        else:
            command = f"GPIO MODE {pin} {mode}"

        return self._send_command(command)

    def gpio_write(self, board: str, pin: int, value: int) -> str:
        """Écrit un niveau logique sur une sortie GPIO de la carte ADC ou PWM."""
        board = board.upper()

        if board not in ("ADC", "PWM"):
            raise ValueError("La carte doit être ADC ou PWM.")

        if not 0 <= pin <= 7:
            raise ValueError("La broche GPIO doit être entre 0 et 7.")

        if value not in (0, 1):
            raise ValueError("La valeur doit être 0 ou 1.")

        if board == "ADC":
            command = f"GPIO ADC WRITE {pin} {value}"
        else:
            command = f"GPIO WRITE {pin} {value}"

        return self._send_command(command)

    def gpio_read(self, board: str, pin: int) -> int:
        """Lit une entrée GPIO sur la carte ADC ou PWM."""
        board = board.upper()

        if board not in ("ADC", "PWM"):
            raise ValueError("La carte doit être ADC ou PWM.")

        if not 0 <= pin <= 7:
            raise ValueError("La broche GPIO doit être entre 0 et 7.")

        if board == "ADC":
            command = f"GPIO ADC READ {pin}"
        else:
            command = f"GPIO READ {pin}"

        response = self._send_command(command)

        if response not in ("0", "1"):
            raise RuntimeError(f"Réponse GPIO invalide : {response}")

        return int(response)

    def adc_select(self, channel: int) -> str:
        """Sélectionne une entrée du multiplexeur ADC."""
        if not 0 <= channel <= 7:
            raise ValueError("Le canal ADC doit être entre 0 et 7.")

        return self._send_command(f"ADC SELECT {channel}")

    def adc_set_resolution(self, resolution: int) -> str:
        """Configure la résolution du MCP3423."""
        if resolution not in (12, 14, 16, 18):
            raise ValueError(
                "La résolution ADC doit être 12, 14, 16 ou 18 bits."
            )

        response = self._send_command(f"ADC RESOLUTION {resolution}")

        if response != "OK":
            raise RuntimeError(
                f"Erreur de configuration ADC : {response}"
            )

        self.adc_resolution = resolution

        return response

    def adc_read(self) -> int:
        """Effectue une conversion ADC et retourne la valeur brute."""
        response = self._send_command("ADC READ")

        if response in ("ERROR", "TIMEOUT"):
            raise RuntimeError(f"Erreur de conversion ADC : {response}")

        try:
            return int(response)
        except ValueError:
            raise RuntimeError(
                f"Réponse ADC invalide : {response}"
            )

    def adc_capture(self, duration_ms: int = 3000) -> list[tuple[int, int]]:
        """Acquiert les échantillons ADC et leurs horodatages."""

        if duration_ms != 3000:
            raise ValueError("Seule une durée de 3000 ms est supportée.")

        with self._lock:
            response = self._send_command(
                f"ADC CAPTURE {duration_ms}",
                timeout=5.0
            )

            if not response.startswith("SAMPLES "):
                raise RuntimeError(f"Erreur d'acquisition : {response}")

            count = int(response.split()[1])
            samples = []

            for index in range(0, count, 4):
                response = self._send_command(f"ADC BLOCK {index}")

                if response == "ERROR":
                    raise RuntimeError(f"Erreur au bloc {index}")

                for item in response.split(";"):
                    if not item:
                        continue

                    timestamp, raw = map(int, item.split(","))
                    samples.append((timestamp, raw))

            if len(samples) != count:
                raise RuntimeError(
                    f"Nombre d'échantillons incorrect : "
                    f"{len(samples)} reçus sur {count}"
                )

            return samples

    def adc_read_voltage(self) -> float:
        """Lit la tension ADC selon la résolution configurée."""
        lsb = {
            12: 0.001,
            14: 0.00025,
            16: 0.0000625,
            18: 0.000015625,
        }

        raw = self.adc_read()

        return raw * lsb[self.adc_resolution]