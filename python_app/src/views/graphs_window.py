from PyQt6.QtWidgets import (
    QWidget,
    QVBoxLayout,
    QScrollArea,
)

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure


class GraphsWindow(QWidget):
    """Fenêtre affichant les signaux mesurés."""

    def __init__(self):
        super().__init__()

        self.setWindowTitle("Lit de clous - Signaux mesurés")
        self.resize(1100, 900)

        layout = QVBoxLayout(self)

        # Zone défilante
        self.scroll_area = QScrollArea()
        self.scroll_area.setWidgetResizable(True)

        self.graphs_container = QWidget()
        self.graphs_layout = QVBoxLayout(self.graphs_container)
        self.graphs_layout.setSpacing(15)
        self.graphs_layout.setContentsMargins(10, 10, 10, 10)
        self.graphs_layout.addStretch()

        self.scroll_area.setWidget(self.graphs_container)
        layout.addWidget(self.scroll_area)

        # Dictionnaire des graphiques par fréquence
        self.graphs = {}

    def plot_signal(self, frequency: int, time: list, voltage: list):
        """Affiche ou met à jour le graphique d'une fréquence."""

        if frequency not in self.graphs:

            figure = Figure(figsize=(10, 2.5))
            canvas = FigureCanvasQTAgg(figure)
            ax = figure.add_subplot(111)

            canvas.setFixedHeight(280)

            self.graphs[frequency] = {
                "figure": figure,
                "canvas": canvas,
                "ax": ax,
            }

            # Insérer le graphique en ordre croissant
            position = sum(
                existing_frequency < frequency
                for existing_frequency in self.graphs
            )

            self.graphs_layout.insertWidget(position, canvas)

        graph = self.graphs[frequency]
        ax = graph["ax"]

        ax.clear()

        ax.plot(
            time,
            voltage,
            marker=".",
            markersize=4,
            linewidth=1
        )

        # Afficher cinq périodes
        display_duration = 5 / frequency
        ax.set_xlim(0, display_duration)

        ax.set_title(f"Signal à {frequency} Hz")
        ax.set_ylabel("Tension (V)")
        ax.set_xlabel("Temps (s)")
        ax.grid(True)

        graph["figure"].tight_layout()
        graph["canvas"].draw_idle()

    def clear_graphs(self):
        """Supprime tous les graphiques."""

        for graph in self.graphs.values():
            canvas = graph["canvas"]
            self.graphs_layout.removeWidget(canvas)
            canvas.deleteLater()
            graph["figure"].clear()

        self.graphs.clear()