from PyQt6.QtWidgets import QMessageBox

from src.hardware.stm32_hardware import STM32Hardware
from src.config import STM32_PORT

from PyQt6.QtWidgets import QTableWidgetItem
from src.models.test_runner import TestRunner

from src.controllers.test_worker import TestWorker

import csv
from pathlib import Path

from PyQt6.QtWidgets import QFileDialog, QMessageBox


class MainController:
    """Contrôleur principal de l'application."""

    def __init__(self, view):
        self.view = view
        self.stm32 = STM32Hardware(port=STM32_PORT)
        self.current_test = 1
        self.test_runner = TestRunner(self.stm32)
        self.view.connect_button.clicked.connect(self.connect_stm32)
        self.view.test_button.clicked.connect(self.run_tests)
        self.view.export_button.clicked.connect(self.export_results)
        self.worker = None
        self.filter_results = {}
        self.test_results = {}
        self.view.restart_button.clicked.connect(self.restart_tests)

    def connect_stm32(self):
        """Établit la connexion USB avec le STM32."""

        try:
            self.stm32.connect()

            response = self.stm32.ping()

            if response != "PONG":
                raise RuntimeError(
                    f"Réponse inattendue du STM32 : {response}"
                )

            self.view.connect_button.setText("STM32 connecté")
            self.view.status_label.setText("État : Connecté")
            self.view.connect_button.setEnabled(False)
            self.view.test_button.setEnabled(True)
            self.view.restart_button.setEnabled(True)

        except Exception as error:
            self.stm32.disconnect()
            self.view.status_label.setText("État : Erreur de connexion")

            QMessageBox.critical(
                self.view,
                "Erreur de connexion",
                str(error)
            )

    def disconnect_stm32(self):
        """Ferme la connexion USB."""
        self.stm32.disconnect()
        self.view.status_label.setText("État : Déconnecté")

    def run_tests(self):
        """Lance une seule étape de test à chaque clic."""

        self.view.test_button.setEnabled(False)
        self.view.restart_button.setEnabled(False)
        self.view.frequency_spinbox.setEnabled(False)

        test_number = self.current_test

        self.view.results_table.setItem(
            test_number - 1,
            2,
            QTableWidgetItem("En cours")
        )

        if test_number == 3:
            frequency = self.view.frequency_spinbox.value()

            self.worker = TestWorker(
                self.test_runner,
                test_number=3,
                frequency=frequency
            )
        else:
            self.worker = TestWorker(
                self.test_runner,
                test_number=test_number
            )

        self.worker.test_completed.connect(self.on_test_completed)
        self.worker.test_failed.connect(self.on_test_failed)

        self.worker.start()

    def on_test_completed(self, test_number, result):
        """Affiche le résultat et prépare l'étape suivante."""

        row = test_number - 1

        # Vérifier si le test a retourné un résultat
        if result is None:
            self.view.results_table.setItem(
                row, 1, QTableWidgetItem("Aucun résultat")
            )

            self.view.results_table.setItem(
                row, 2, QTableWidgetItem("Non complété")
            )

            self.view.test_button.setEnabled(True)
            self.view.restart_button.setEnabled(True)

            return

        if test_number in (1, 2):
            self.test_results[test_number] = result

        if test_number == 1:
            result_text = f"{result:.3f} V"

        elif test_number == 2:
            frequency = result["frequency_measured"]
            duty = result["duty_measured"]

            duty_text = (
                f"{duty:.1f} %"
                if duty is not None else "Non mesurable"
            )

            result_text = f"{frequency:.2f} Hz - {duty_text}"

        elif test_number == 3:
            frequency = result["frequency"]

            # Conserver les données de cette fréquence
            self.filter_results[frequency] = result

            # Afficher la courbe
            self.view.graphs_window.plot_signal(
                frequency=frequency,
                time=result["time"],
                voltage=result["voltage"]
            )

            result_text = (
                f"{len(self.filter_results)} fréquence(s) mesurée(s)"
            )

        else:
            return

        self.view.results_table.setItem(
            row, 1, QTableWidgetItem(result_text)
        )

        self.view.results_table.setItem(
            row, 2, QTableWidgetItem("Terminé")
        )

        if test_number < 3:
            self.current_test = test_number + 1

            if self.current_test == 3:
                self.view.frequency_spinbox.setEnabled(True)
                self.view.test_button.setText("Mesurer")
            else:
                self.view.frequency_spinbox.setEnabled(False)
                self.view.test_button.setText("Test suivant")

        else:
            self.view.test_button.setText("Mesurer")

        self.view.test_button.setEnabled(True)
        self.view.restart_button.setEnabled(True)
        if self.current_test == 3:
            self.view.frequency_spinbox.setEnabled(True)


    def on_test_failed(self, test_number, error):
        """Affiche une erreur d'exécution."""

        row = test_number - 1

        self.view.results_table.setItem(
            row, 1, QTableWidgetItem(error)
        )
        
        self.view.results_table.setItem(
            row, 2, QTableWidgetItem("Erreur")
        )

        self.view.test_button.setEnabled(True)
        self.view.restart_button.setEnabled(True)
        if self.current_test == 3:
            self.view.frequency_spinbox.setEnabled(True)

        QMessageBox.critical(
            self.view,
            "Erreur du test",
            error
        )

    def export_results(self):
        """Exporte les résultats des tests dans deux fichiers CSV."""

        if not self.test_results and not getattr(self, "filter_results", {}):
            QMessageBox.information(
                self.view,
                "Exportation",
                "Aucun résultat à exporter."
            )
            return

        folder = QFileDialog.getExistingDirectory(
            self.view,
            "Choisir le dossier d'exportation"
        )

        if not folder:
            return

        folder = Path(folder)

        try:
            # Fichier 1 : résultats des tests 1 et 2
            with open(
                folder / "resultats_tests.csv",
                "w",
                newline="",
                encoding="utf-8-sig"
            ) as file:
                writer = csv.writer(file, delimiter=";")
                writer.writerow(["Test", "Parametre", "Valeur", "Unite"])

                if 1 in self.test_results:
                    writer.writerow([
                        1, "Tension ADC",
                        self.test_results[1], "V"
                    ])

                if 2 in self.test_results:
                    result = self.test_results[2]

                    writer.writerow([
                        2, "Frequence PWM mesuree",
                        result["frequency_measured"], "Hz"
                    ])

                    writer.writerow([
                        2, "Rapport cyclique mesure",
                        result["duty_measured"], "%"
                    ])

            # Fichier 2 : acquisitions du test 3
            with open(
                folder / "acquisitions_frequentielles.csv",
                "w",
                newline="",
                encoding="utf-8-sig"
            ) as file:
                writer = csv.writer(file, delimiter=";")
                writer.writerow([
                    "Frequence_Hz",
                    "Temps_s",
                    "ADC_brut",
                    "Tension_V"
                ])

                for frequency, result in sorted(
                    getattr(self, "filter_results", {}).items()
                ):
                    for (timestamp, raw), voltage in zip(
                        result["samples"],
                        result["voltage"]
                    ):
                        writer.writerow([
                            frequency,
                            timestamp / 1000.0,
                            raw,
                            voltage
                        ])

        except OSError as error:
            QMessageBox.critical(
                self.view,
                "Erreur d'exportation",
                str(error)
            )
            return

        QMessageBox.information(
            self.view,
            "Exportation",
            "Les deux fichiers CSV ont été créés."
        )


    def restart_tests(self):
        """Réinitialise la séquence de tests."""

        if self.worker is not None and self.worker.isRunning():
            return

        self.current_test = 1

        self.test_results.clear()
        self.filter_results.clear()
        self.view.graphs_window.clear_graphs()

        for row in range(3):
            self.view.results_table.setItem(
                row, 1, QTableWidgetItem("—")
            )
            self.view.results_table.setItem(
                row, 2, QTableWidgetItem("En attente")
            )

        self.view.frequency_spinbox.setValue(10)
        self.view.frequency_spinbox.setEnabled(False)

        self.view.test_button.setText("Exécuter les tests")
        self.view.test_button.setEnabled(True)