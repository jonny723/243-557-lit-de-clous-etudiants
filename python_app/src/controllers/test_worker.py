from PyQt6.QtCore import QThread, pyqtSignal


class TestWorker(QThread):
    """Exécute les tests sans bloquer l'interface graphique."""

    test_completed = pyqtSignal(int, object)
    test_failed = pyqtSignal(int, str)

    def __init__(
        self,
        test_runner,
        test_number: int,
        frequency: int | None = None
    ):
        super().__init__()

        self.test_runner = test_runner
        self.test_number = test_number
        self.frequency = frequency

    def run(self):
        try:
            if self.test_number == 1:
                result = self.test_runner.test_voltage()

            elif self.test_number == 2:
                result = self.test_runner.test_frequency()

            elif self.test_number == 3:
                if self.frequency is None:
                    raise ValueError(
                        "Fréquence requise pour le test 3"
                    )

                result = self.test_runner.test_frequency_response(
                    frequency=self.frequency
                )

            else:
                raise ValueError("Numéro de test invalide")

            self.test_completed.emit(self.test_number, result)

        except Exception as error:
            self.test_failed.emit(self.test_number, str(error))