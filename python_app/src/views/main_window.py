from PyQt6.QtWidgets import (
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QLabel,
    QPushButton,
    QGroupBox,
    QTableWidget,
    QTableWidgetItem,
    QHeaderView,
    QSpinBox,
)

from src.views.graphs_window import GraphsWindow


class MainWindow(QMainWindow):
    """Vue principale de l'application de test."""

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Lit de clous - Application de test")
        self.resize(1000, 700)

        central_widget = QWidget()
        self.setCentralWidget(central_widget)

        layout = QVBoxLayout(central_widget)

        title = QLabel("Système de test - Lit de clous")
        layout.addWidget(title)

        self.connect_button = QPushButton("Connecter au STM32")
        layout.addWidget(self.connect_button)

        self.status_label = QLabel("État : Déconnecté")
        layout.addWidget(self.status_label)

        self.test_button = QPushButton("Exécuter les tests")
        self.test_button.setEnabled(False)
        layout.addWidget(self.test_button)

        # Commandes de gestion des tests

        self.restart_button = QPushButton("Recommencer les tests")
        self.restart_button.setEnabled(False)

        layout.addWidget(self.restart_button)

        # Sélection de la fréquence pour le test 3
        self.frequency_label = QLabel("Fréquence du test 3 :")

        self.frequency_spinbox = QSpinBox()
        self.frequency_spinbox.setRange(1, 100)
        self.frequency_spinbox.setValue(10)
        self.frequency_spinbox.setSuffix(" Hz")
        self.frequency_spinbox.setSingleStep(1)
        self.frequency_spinbox.setEnabled(False)

        layout.addWidget(self.frequency_label)
        layout.addWidget(self.frequency_spinbox)

  
        # Section des tests
        tests_group = QGroupBox("Séquences de tests")
        tests_layout = QVBoxLayout(tests_group)

        self.test_voltage_label = QLabel("1. Mesure de tension ADC")
        self.test_frequency_label = QLabel("2. Mesure de fréquence PWM - 10 Hz")
        self.test_filter_label = QLabel(
            "3. Réponse fréquentielle - Fréquence réglable de 1 à 100 Hz"
        )

        tests_layout.addWidget(self.test_voltage_label)
        tests_layout.addWidget(self.test_frequency_label)
        tests_layout.addWidget(self.test_filter_label)

        layout.addWidget(tests_group)

        # Tableau des résultats
        results_group = QGroupBox("Résultats des tests")
        results_layout = QVBoxLayout(results_group)

        self.results_table = QTableWidget(3, 3)

        self.results_table.setHorizontalHeaderLabels([
            "Test",
            "Résultat",
            "État"
        ])

        test_names = [
            "Mesure de tension ADC",
            "Mesure de fréquence PWM",
            "Réponse fréquentielle"
        ]

        for row, name in enumerate(test_names):
            self.results_table.setItem(row, 0, QTableWidgetItem(name))
            self.results_table.setItem(row, 1, QTableWidgetItem("—"))
            self.results_table.setItem(row, 2, QTableWidgetItem("En attente"))

        header = self.results_table.horizontalHeader()

        header.setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.ResizeToContents)

        results_layout.addWidget(self.results_table)
        layout.addWidget(results_group)

        # Fenêtre indépendante des graphiques
        self.graphs_window = GraphsWindow()

        self.graphs_button = QPushButton("Afficher les graphiques")
        self.graphs_button.clicked.connect(self.graphs_window.show)

        layout.addWidget(self.graphs_button)

        self.export_button = QPushButton("Exporter les résultats en CSV")
        layout.addWidget(self.export_button)

        layout.addStretch()