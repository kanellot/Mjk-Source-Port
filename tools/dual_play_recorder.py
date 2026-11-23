#!/usr/bin/env python3
"""
tools/dual_play_recorder.py - Laboratorio Interactivo Dual (PySide6 / Qt6)

Permite jugar a la ROM original de NES (Golden Anchor) con teclado o mando
mientras el Source Port en C99 corre simultáneamente como reflejo en tiempo real.

LA ROM ORIGINAL (CYNES) ES LA REFERENCIA ABSOLUTA:
Todas las pulsaciones de mando y teclado se inyectan en formato cynes.NES_INPUT_*.
El motor traduce la máscara automáticamente para el Port en C.
"""

import os
import sys
import json
import time
import ctypes
from datetime import datetime
import numpy as np

try:
    from PySide6 import QtCore, QtGui, QtWidgets
except ImportError:
    print("[ERROR]: Se requiere PySide6 (Qt6).")
    sys.exit(1)

try:
    import cynes
except ImportError:
    print("[ERROR]: Se requiere la biblioteca 'cynes' de Python.")
    sys.exit(1)

# Asegurar que el directorio de tools esté en sys.path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hierarchical_parity_oracle import HierarchicalOracle, CYNES_BTN_A, CYNES_BTN_B, CYNES_BTN_SELECT, CYNES_BTN_START, CYNES_BTN_UP, CYNES_BTN_DOWN, CYNES_BTN_LEFT, CYNES_BTN_RIGHT

# Soporte XInput para mandos Xbox en Windows
class XINPUT_GAMEPAD(ctypes.Structure):
    _fields_ = [
        ('wButtons', ctypes.c_ushort),
        ('bLeftTrigger', ctypes.c_ubyte),
        ('bRightTrigger', ctypes.c_ubyte),
        ('sThumbLX', ctypes.c_short),
        ('sThumbLY', ctypes.c_short),
        ('sThumbRX', ctypes.c_short),
        ('sThumbRY', ctypes.c_short),
    ]

class XINPUT_STATE(ctypes.Structure):
    _fields_ = [
        ('dwPacketNumber', ctypes.c_ulong),
        ('Gamepad', XINPUT_GAMEPAD),
    ]

_xinput = None
for _dll in ['xinput1_4.dll', 'xinput1_3.dll', 'xinput9_1_0.dll']:
    try:
        _xinput = ctypes.windll.LoadLibrary(_dll)
        break
    except Exception:
        pass

def poll_xinput_controller() -> int:
    """Lee el estado del mando 0 mediante XInput y lo convierte a máscara cynes."""
    if not _xinput:
        return 0
    state = XINPUT_STATE()
    if _xinput.XInputGetState(0, ctypes.byref(state)) != 0:
        return 0 # No conectado

    pad = state.Gamepad
    btn = pad.wButtons
    mask = 0

    # D-Pad y Stick analógico izquierdo
    if (btn & 0x0001) or pad.sThumbLY > 16000:
        mask |= CYNES_BTN_UP
    if (btn & 0x0002) or pad.sThumbLY < -16000:
        mask |= CYNES_BTN_DOWN
    if (btn & 0x0004) or pad.sThumbLX < -16000:
        mask |= CYNES_BTN_LEFT
    if (btn & 0x0008) or pad.sThumbLX > 16000:
        mask |= CYNES_BTN_RIGHT

    # Botones A, B, X
    if (btn & 0x1000): # Botón A de Xbox
        mask |= CYNES_BTN_A
    if (btn & 0x2000) or (btn & 0x4000): # Botón B o X de Xbox
        mask |= CYNES_BTN_B

    # Start y Select/Back
    if (btn & 0x0010): # Start
        mask |= CYNES_BTN_START
    if (btn & 0x0020): # Back
        mask |= CYNES_BTN_SELECT

    return mask

class DualPlayWindow(QtWidgets.QMainWindow):
    def __init__(self, rom_path="asm/bin/mbj-us.nes", dll_path="bin/mjk_core.dll"):
        super().__init__()
        self.setWindowTitle("Mighty Bomb Jack - Laboratorio Dual (Referencia Canónica: ROM Original NES)")
        self.resize(1080, 700)

        self.rom_path = rom_path
        self.dll_path = dll_path

        # Inicializar el motor dual (cynes es la referencia)
        self.oracle = HierarchicalOracle(rom_path=rom_path, dll_path=dll_path)
        self.frame_count = 0
        self.is_paused = False
        self.keyboard_mask = 0
        self.recorded_inputs = [] # Lista de (frame, cynes_input_mask)
        self.divergence_info = None

        self._setup_ui()

        # Timer a 60 Hz (16.66 ms)
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self._game_tick)
        self.timer.start(16)

    def _setup_ui(self):
        central_widget = QtWidgets.QWidget(self)
        self.setCentralWidget(central_widget)
        main_layout = QtWidgets.QVBoxLayout(central_widget)
        main_layout.setContentsMargins(12, 10, 12, 10)
        main_layout.setSpacing(10)

        # 1. Barra Superior de Estado y Quality Gates
        top_bar = QtWidgets.QHBoxLayout()
        self.lbl_frame = QtWidgets.QLabel("<b>Frame:</b> 0 (00:00.00)")
        self.lbl_status = QtWidgets.QLabel("<span style='color: #4CAF50;'><b>ESTADO: SINCRONIZADO (100% PARIDAD)</b></span>")
        self.lbl_buttons = QtWidgets.QLabel("<b>Inputs:</b> [ ]")
        top_bar.addWidget(self.lbl_frame)
        top_bar.addSpacing(20)
        top_bar.addWidget(self.lbl_status)
        top_bar.addStretch()
        top_bar.addWidget(self.lbl_buttons)
        main_layout.addLayout(top_bar)

        # 2. Viewports de Juego (Pantalla Dividida)
        viewports_layout = QtWidgets.QHBoxLayout()

        # Viewport Izquierdo: NES Original (cynes)
        vbox_left = QtWidgets.QVBoxLayout()
        lbl_nes_title = QtWidgets.QLabel("<b>[1] GOLDEN REFERENCE (ROM NES 1987)</b> — <i>Control Principal</i>")
        lbl_nes_title.setAlignment(QtCore.Qt.AlignCenter)
        lbl_nes_title.setStyleSheet("color: #4CAF50; font-size: 13px; font-weight: bold;")
        self.screen_nes = QtWidgets.QLabel()
        self.screen_nes.setFixedSize(512, 480)
        self.screen_nes.setStyleSheet("border: 3px solid #4CAF50; background-color: #000;")
        self.screen_nes.setScaledContents(True)
        vbox_left.addWidget(lbl_nes_title)
        vbox_left.addWidget(self.screen_nes)
        viewports_layout.addLayout(vbox_left)

        # Viewport Derecho: Port en C99
        vbox_right = QtWidgets.QVBoxLayout()
        lbl_port_title = QtWidgets.QLabel("<b>[2] SOURCE PORT C99 (NATIVO WINDOWS)</b> — <i>Reflejo Espejo</i>")
        lbl_port_title.setAlignment(QtCore.Qt.AlignCenter)
        lbl_port_title.setStyleSheet("color: #2196F3; font-size: 13px; font-weight: bold;")
        self.screen_port = QtWidgets.QLabel()
        self.screen_port.setFixedSize(512, 480)
        self.screen_port.setStyleSheet("border: 3px solid #2196F3; background-color: #000;")
        self.screen_port.setScaledContents(True)
        vbox_right.addWidget(lbl_port_title)
        vbox_right.addWidget(self.screen_port)
        viewports_layout.addLayout(vbox_right)

        main_layout.addLayout(viewports_layout)

        # 3. Panel Inferior de Telemetría Comparativa
        self.telemetry_box = QtWidgets.QGroupBox("Telemetría en Vivo de Estados de Memoria")
        telem_layout = QtWidgets.QHBoxLayout(self.telemetry_box)

        self.lbl_telem_nes = QtWidgets.QLabel("NES: InGame=0 | State=0 | Room=01 | Jack=(0,0)")
        self.lbl_telem_port = QtWidgets.QLabel("PORT: InGame=0 | State=0 | Room=01 | Jack=(0,0)")
        self.lbl_controls_hint = QtWidgets.QLabel("<b>Teclado / Mando:</b> Flechas/WASD | Z/J=A (Salto) | X/K=B | Enter=START | Espacio=Pausa | G=Guardar TAS")
        self.lbl_controls_hint.setStyleSheet("color: #AAA;")

        telem_layout.addWidget(self.lbl_telem_nes)
        telem_layout.addSpacing(25)
        telem_layout.addWidget(self.lbl_telem_port)
        telem_layout.addStretch()
        telem_layout.addWidget(self.lbl_controls_hint)

        main_layout.addWidget(self.telemetry_box)

    def _get_active_input_mask(self) -> int:
        """Combina la entrada del teclado y del mando Xbox."""
        xinput_mask = poll_xinput_controller()
        return self.keyboard_mask | xinput_mask

    def _format_input_labels(self, mask: int) -> str:
        names = []
        if mask & CYNES_BTN_UP:     names.append("ARRIBA")
        if mask & CYNES_BTN_DOWN:   names.append("ABAJO")
        if mask & CYNES_BTN_LEFT:   names.append("IZQ")
        if mask & CYNES_BTN_RIGHT:  names.append("DER")
        if mask & CYNES_BTN_A:      names.append("A")
        if mask & CYNES_BTN_B:      names.append("B")
        if mask & CYNES_BTN_START:  names.append("START")
        if mask & CYNES_BTN_SELECT: names.append("SELECT")
        return " ".join(names) if names else "—"

    def _game_tick(self):
        if self.is_paused:
            return
        self._step_frame()

    def _step_frame(self):
        input_mask = self._get_active_input_mask()
        self.lbl_buttons.setText(f"<b>Inputs:</b> [{self._format_input_labels(input_mask)}]")

        # 1. Registrar input en la sesión TAS
        self.recorded_inputs.append((self.frame_count, input_mask))

        # 2. Avanzar la simulación (la ROM en cynes recibe input_mask canónico directamente)
        nes_pixels = self.oracle.step(input_mask)
        port_fb_ptr = self.oracle.lib.MjkOracle_GetFramebuffer(self.oracle.oracle)

        # 3. Renderizar imagen NES (256x240 RGB)
        nes_img = QtGui.QImage(nes_pixels.data, 256, 240, 256 * 3, QtGui.QImage.Format_RGB888)
        self.screen_nes.setPixmap(QtGui.QPixmap.fromImage(nes_img))

        # 4. Renderizar imagen Port C (256x240 ARGB32)
        port_bytes = ctypes.string_at(port_fb_ptr, 256 * 240 * 4)
        port_img = QtGui.QImage(port_bytes, 256, 240, 256 * 4, QtGui.QImage.Format_ARGB32)
        self.screen_port.setPixmap(QtGui.QPixmap.fromImage(port_img))

        # 5. Extraer y mostrar telemetría
        nes_telem = self.oracle.get_nes_telemetry()
        port_snap = self.oracle.port_snap

        self.frame_count += 1
        seconds = self.frame_count / 60.0
        self.lbl_frame.setText(f"<b>Frame:</b> {self.frame_count} ({seconds:05.2f}s)")

        self.lbl_telem_nes.setText(f"<b>NES:</b> InGame={nes_telem['in_game']} | State={nes_telem['game_state']} | Room={nes_telem['room_id']:02d} | Jack=({nes_telem['jack_x']},{nes_telem['jack_y']})")
        self.lbl_telem_port.setText(f"<b>PORT:</b> InGame={port_snap.in_game} | State={port_snap.game_state} | Room={port_snap.current_room:02d} | Jack=({port_snap.jack_x},{port_snap.jack_y})")

        # 6. Evaluar compuertas jerárquicas
        if not self.divergence_info:
            div = self.oracle.evaluate_quality_gates(max_level=1)
            if div:
                self.divergence_info = div
                self.lbl_status.setText(f"<span style='color: #F44336;'><b>DIVERGENCIA NIVEL {div['failed_level']} EN FRAME {div['frame']}: {div['cause']}</b></span>")
                self.screen_port.setStyleSheet("border: 3px solid #F44336; background-color: #000;")

    def keyPressEvent(self, event: QtGui.QKeyEvent):
        key = event.key()
        if key in (QtCore.Qt.Key_Left, QtCore.Qt.Key_A):
            self.keyboard_mask |= CYNES_BTN_LEFT
        elif key in (QtCore.Qt.Key_Right, QtCore.Qt.Key_D):
            self.keyboard_mask |= CYNES_BTN_RIGHT
        elif key in (QtCore.Qt.Key_Up, QtCore.Qt.Key_W):
            self.keyboard_mask |= CYNES_BTN_UP
        elif key in (QtCore.Qt.Key_Down, QtCore.Qt.Key_S):
            self.keyboard_mask |= CYNES_BTN_DOWN
        elif key in (QtCore.Qt.Key_Z, QtCore.Qt.Key_J):
            self.keyboard_mask |= CYNES_BTN_A
        elif key in (QtCore.Qt.Key_X, QtCore.Qt.Key_K):
            self.keyboard_mask |= CYNES_BTN_B
        elif key in (QtCore.Qt.Key_Return, QtCore.Qt.Key_Enter):
            self.keyboard_mask |= CYNES_BTN_START
        elif key == QtCore.Qt.Key_Shift:
            self.keyboard_mask |= CYNES_BTN_SELECT
        elif key == QtCore.Qt.Key_Space:
            self.is_paused = not self.is_paused
        elif key == QtCore.Qt.Key_F6:
            if self.is_paused:
                self._step_frame()
        elif key == QtCore.Qt.Key_F5:
            self._reset()
        elif key == QtCore.Qt.Key_G:
            self._save_tas_session()

    def keyReleaseEvent(self, event: QtGui.QKeyEvent):
        if event.isAutoRepeat():
            return
        key = event.key()
        if key in (QtCore.Qt.Key_Left, QtCore.Qt.Key_A):
            self.keyboard_mask &= ~CYNES_BTN_LEFT
        elif key in (QtCore.Qt.Key_Right, QtCore.Qt.Key_D):
            self.keyboard_mask &= ~CYNES_BTN_RIGHT
        elif key in (QtCore.Qt.Key_Up, QtCore.Qt.Key_W):
            self.keyboard_mask &= ~CYNES_BTN_UP
        elif key in (QtCore.Qt.Key_Down, QtCore.Qt.Key_S):
            self.keyboard_mask &= ~CYNES_BTN_DOWN
        elif key in (QtCore.Qt.Key_Z, QtCore.Qt.Key_J):
            self.keyboard_mask &= ~CYNES_BTN_A
        elif key in (QtCore.Qt.Key_X, QtCore.Qt.Key_K):
            self.keyboard_mask &= ~CYNES_BTN_B
        elif key in (QtCore.Qt.Key_Return, QtCore.Qt.Key_Enter):
            self.keyboard_mask &= ~CYNES_BTN_START
        elif key == QtCore.Qt.Key_Shift:
            self.keyboard_mask &= ~CYNES_BTN_SELECT

    def _reset(self):
        self.oracle.close()
        self.oracle = HierarchicalOracle(rom_path=self.rom_path, dll_path=self.dll_path)
        self.frame_count = 0
        self.keyboard_mask = 0
        self.recorded_inputs = []
        self.divergence_info = None
        self.lbl_status.setText("<span style='color: #4CAF50;'><b>ESTADO: SINCRONIZADO (100% PARIDAD)</b></span>")
        self.screen_port.setStyleSheet("border: 3px solid #2196F3; background-color: #000;")

    def _save_tas_session(self):
        out_dir = "tests/tas"
        os.makedirs(out_dir, exist_ok=True)
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        tas_file = os.path.join(out_dir, f"session_{ts}.tas")

        with open(tas_file, "w", encoding="utf-8") as f:
            f.write("# Mighty Bomb Jack Canonical TAS Session (CYNES Ground Truth Reference)\n")
            f.write(f"# Date: {datetime.now().isoformat()}\n")
            f.write(f"# Total Frames: {len(self.recorded_inputs)}\n")
            f.write("# Bitmask: RIGHT=1, LEFT=2, DOWN=4, UP=8, START=16, SELECT=32, B=64, A=128\n")
            f.write("# FRAME,BUTTON_BITMASK\n")
            for frame, bitmask in self.recorded_inputs:
                f.write(f"{frame},{bitmask}\n")

        print(f"\n[OK]: Sesión TAS guardada con éxito en {tas_file} ({len(self.recorded_inputs)} frames).")
        QtWidgets.QMessageBox.information(self, "Sesión Guardada", f"Partida TAS grabada con éxito:\n{tas_file}\n\nTotal de frames: {len(self.recorded_inputs)}")

    def closeEvent(self, event):
        self.oracle.close()
        event.accept()

def main():
    app = QtWidgets.QApplication(sys.argv)
    window = DualPlayWindow()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
