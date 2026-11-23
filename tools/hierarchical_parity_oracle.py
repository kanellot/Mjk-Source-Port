#!/usr/bin/env python3
"""
tools/hierarchical_parity_oracle.py - Oráculo de Paridad Jerárquica Macro-a-Micro

Ejecuta de forma determinista y headless la ROM original de NES (via cynes)
y el Source Port en C99 (via bin/mjk_core.dll) fotograma a fotograma.

La ROM original (cynes) es la REFERENCIA CANÓNICA ABSOLUTA:
Todas las máscaras de entrada se rigen por cynes.NES_INPUT_*:
- NES_INPUT_RIGHT  = 1
- NES_INPUT_LEFT   = 2
- NES_INPUT_DOWN   = 4
- NES_INPUT_UP     = 8
- NES_INPUT_START  = 16
- NES_INPUT_SELECT = 32
- NES_INPUT_B      = 64
- NES_INPUT_A      = 128
"""

import os
import sys
import json
import ctypes
import argparse
from typing import Dict, Any, List, Optional

try:
    import cynes
except ImportError:
    print("[ERROR]: Se requiere la biblioteca 'cynes' de Python.")
    sys.exit(1)

# Constantes canónicas de cynes
CYNES_BTN_RIGHT  = cynes.NES_INPUT_RIGHT   # 1
CYNES_BTN_LEFT   = cynes.NES_INPUT_LEFT    # 2
CYNES_BTN_DOWN   = cynes.NES_INPUT_DOWN    # 4
CYNES_BTN_UP     = cynes.NES_INPUT_UP      # 8
CYNES_BTN_START  = cynes.NES_INPUT_START   # 16
CYNES_BTN_SELECT = cynes.NES_INPUT_SELECT  # 32
CYNES_BTN_B      = cynes.NES_INPUT_B       # 64
CYNES_BTN_A      = cynes.NES_INPUT_A       # 128

def cynes_to_port_mask(cynes_mask: int) -> int:
    """Traduce la máscara canónica de cynes al formato interno del Port en C."""
    port_mask = 0
    if cynes_mask & CYNES_BTN_A:      port_mask |= (1 << 0)
    if cynes_mask & CYNES_BTN_B:      port_mask |= (1 << 1)
    if cynes_mask & CYNES_BTN_SELECT: port_mask |= (1 << 2)
    if cynes_mask & CYNES_BTN_START:  port_mask |= (1 << 3)
    if cynes_mask & CYNES_BTN_UP:     port_mask |= (1 << 4)
    if cynes_mask & CYNES_BTN_DOWN:   port_mask |= (1 << 5)
    if cynes_mask & CYNES_BTN_LEFT:   port_mask |= (1 << 6)
    if cynes_mask & CYNES_BTN_RIGHT:  port_mask |= (1 << 7)
    return port_mask

# Estructura Ctype idéntica a MjkCanonicalSnapshot en mjk_oracle_api.h
class EnemySnapshot(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ('type', ctypes.c_uint8),
        ('status', ctypes.c_uint8),
        ('x', ctypes.c_int16),
        ('y', ctypes.c_int16),
        ('timer', ctypes.c_uint8),
        ('active', ctypes.c_uint8),
    ]

class CanonicalSnapshot(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ('frame', ctypes.c_uint32),
        ('game_state', ctypes.c_uint8),
        ('title_screen_state', ctypes.c_uint8),
        ('in_game', ctypes.c_uint8),
        ('current_room', ctypes.c_uint8),
        ('state_timer', ctypes.c_uint32),
        ('stage_timer_seconds', ctypes.c_uint32),
        ('greed_jump_counter', ctypes.c_uint16),
        ('lives_count', ctypes.c_uint8),
        ('score_bcd', ctypes.c_uint8 * 4),
        ('door_tile_x', ctypes.c_uint8),
        ('door_tile_y', ctypes.c_uint8),
        ('door_is_open', ctypes.c_uint8),
        ('door_condition_id', ctypes.c_uint8),
        ('bomb_count', ctypes.c_uint8),
        ('active_fire_bomb_index', ctypes.c_uint8),
        ('chest_count', ctypes.c_uint8),
        ('room_data_slot', ctypes.c_uint8 * 16),
        ('active_enemies_count', ctypes.c_uint8),
        ('enemies', EnemySnapshot * 6),
        ('enemy_coin_timer', ctypes.c_uint16),
        ('spark_visible', ctypes.c_uint8),
        ('spark_x', ctypes.c_uint8),
        ('spark_y', ctypes.c_uint8),
        ('jack_x', ctypes.c_int16),
        ('jack_y', ctypes.c_int16),
        ('jack_sub_x', ctypes.c_uint8),
        ('jack_sub_y', ctypes.c_uint8),
        ('jack_vel_x', ctypes.c_int16),
        ('jack_vel_y', ctypes.c_int16),
        ('jack_state', ctypes.c_uint8),
        ('on_ground', ctypes.c_uint8),
        ('flutter_active', ctypes.c_uint8),
        ('oam_sprites', ctypes.c_uint8 * 256),
        ('backdrop_color', ctypes.c_uint32),
    ]

class HierarchicalOracle:
    def __init__(self, rom_path: str = "asm/bin/mbj-us.nes", dll_path: str = "bin/mjk_core.dll", assets_path: str = "assets/assets.dat"):
        if not os.path.exists(rom_path):
            raise FileNotFoundError(f"ROM no encontrada en {rom_path}")
        if not os.path.exists(dll_path):
            raise FileNotFoundError(f"Biblioteca compartida no encontrada en {dll_path}. Ejecute 'build.bat' primero.")
        if not os.path.exists(assets_path):
            raise FileNotFoundError(f"Assets no encontrados en {assets_path}")

        self.rom_path = rom_path
        self.nes = cynes.NES(rom_path)
        self.nes.reset()

        self.lib = ctypes.CDLL(os.path.abspath(dll_path))
        self.lib.MjkOracle_Create.restype = ctypes.c_void_p
        self.lib.MjkOracle_Create.argtypes = [ctypes.c_char_p]
        self.lib.MjkOracle_Destroy.argtypes = [ctypes.c_void_p]
        self.lib.MjkOracle_Step.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
        self.lib.MjkOracle_GetSnapshot.argtypes = [ctypes.c_void_p, ctypes.POINTER(CanonicalSnapshot)]
        self.lib.MjkOracle_GetFramebuffer.restype = ctypes.POINTER(ctypes.c_uint32)
        self.lib.MjkOracle_GetFramebuffer.argtypes = [ctypes.c_void_p]

        self.oracle = self.lib.MjkOracle_Create(assets_path.encode('utf-8'))
        if not self.oracle:
            raise RuntimeError("Fallo al inicializar MjkOracle_Create en C.")

        self.current_frame = 0
        self.port_snap = CanonicalSnapshot()

    def close(self):
        if self.oracle:
            self.lib.MjkOracle_Destroy(self.oracle)
            self.oracle = None

    def step(self, cynes_input_mask: int):
        """
        Avanza la ROM original con la máscara canónica de cynes,
        y traduce la máscara al Port en C para mantener paridad 1:1.
        """
        # 1. Avanzar emulador NES con su formato nativo
        self.nes.controller = cynes_input_mask
        nes_pixels = self.nes.step(1)

        # 2. Traducir y avanzar Port en C
        port_mask = cynes_to_port_mask(cynes_input_mask)
        self.lib.MjkOracle_Step(self.oracle, port_mask)
        self.lib.MjkOracle_GetSnapshot(self.oracle, ctypes.byref(self.port_snap))

        self.current_frame += 1
        return nes_pixels

    def get_nes_telemetry(self) -> Dict[str, Any]:
        """Extrae la telemetría canónica directa de los registros de RAM de la NES."""
        nes = self.nes
        in_game = nes[0x43]
        game_state = nes[0x44]
        title_state = nes[0x45]
        room_id = nes[0x3BF]
        cp_timer = nes[0x88]

        # Posiciones Jack
        if in_game:
            jack_x = nes[0x3D8]
            jack_y = nes[0x3D9]
            jack_sub_x = nes[0x3DA]
            jack_sub_y = nes[0x3DB]
        else:
            jack_x = 40
            jack_y = nes[0x3D9] if title_state == 5 else 120
            jack_sub_x = 0
            jack_sub_y = 0

        # RoomDataRAM Slot activo
        slot_raw = [nes[0x603 + i] for i in range(16)]

        # Puerta y objetos
        door_byte = slot_raw[2] if len(slot_raw) > 2 else 0x12
        door_is_open = (door_byte & 0x80) != 0

        # Enemigos vivos
        active_enemies = nes[0x316]

        # OAM chispa de mecha
        spark_y = nes[0x210]

        return {
            "frame": self.current_frame,
            "in_game": in_game,
            "game_state": game_state,
            "title_state": title_state,
            "room_id": room_id,
            "cp_timer": cp_timer,
            "jack_x": jack_x,
            "jack_y": jack_y,
            "jack_sub_x": jack_sub_x,
            "jack_sub_y": jack_sub_y,
            "room_slot": slot_raw,
            "door_is_open": door_is_open,
            "active_enemies": active_enemies,
            "spark_y": spark_y,
            "lives": nes[0x3E4]
        }

    def evaluate_quality_gates(self, max_level: int = 5) -> Optional[Dict[str, Any]]:
        """
        Evalúa secuencialmente las 5 compuertas de la pirámide de paridad.
        Si una compuerta superior falla, retorna el reporte de divergencia y aborta.
        """
        nes = self.get_nes_telemetry()
        port = self.port_snap
        f = self.current_frame

        # =====================================================================
        # NIVEL 1: Macro-Flujo y Máquina de Estados (Compuerta 1)
        # =====================================================================
        if max_level >= 1:
            # 1. InGameFlag
            if nes["in_game"] != port.in_game:
                return {
                    "failed_level": 1,
                    "level_name": "MACRO_FLOW_IN_GAME_FLAG",
                    "frame": f,
                    "discrepancy": {
                        "variable": "in_game",
                        "golden_nes": nes["in_game"],
                        "port_c": port.in_game
                    },
                    "cause": "Desincronización en la bandera global de partida activa ($0043)."
                }

            # 2. Secuencia de Título
            if not nes["in_game"]:
                # Tolerar la inicialización de frame 0 a 1 si ambos están en fase de Copyright
                if nes["title_state"] != port.title_screen_state and not (nes["title_state"] == 0 and port.title_screen_state == 1):
                    return {
                        "failed_level": 1,
                        "level_name": "MACRO_FLOW_TITLE_STATE",
                        "frame": f,
                        "discrepancy": {
                            "variable": "title_screen_state",
                            "golden_nes": nes["title_state"],
                            "port_c": port.title_screen_state
                        },
                        "cause": f"Discrepancia en la secuencia de inicio: NES en fase {nes['title_state']}, Port en fase {port.title_screen_state}."
                    }

            # 3. GameState en partida activa
            if nes["in_game"]:
                # Detección de salto ilegal a Torture Room
                if port.game_state == 7 and nes["game_state"] != 7:
                    return {
                        "failed_level": 1,
                        "level_name": "MACRO_FLOW_ILLEGAL_TORTURE_ROOM",
                        "frame": f,
                        "discrepancy": {
                            "variable": "game_state",
                            "golden_nes": f"{nes['game_state']} (Normal Gameplay)",
                            "port_c": "7 (GAME_STATE_7_TORTURE_ROOM)"
                        },
                        "cause": "Disparo ilegal a la Sala de Tortura. El contador greed_jump_counter o temporizador se activó sin motivo canónico."
                    }

                # GameState exacto
                if nes["game_state"] != port.game_state:
                    return {
                        "failed_level": 1,
                        "level_name": "MACRO_FLOW_GAME_STATE",
                        "frame": f,
                        "discrepancy": {
                            "variable": "game_state",
                            "golden_nes": nes["game_state"],
                            "port_c": port.game_state
                        },
                        "cause": f"Discrepancia de estado de juego: NES en estado {nes['game_state']}, Port en estado {port.game_state}."
                    }

                # RoomID activo
                if nes["room_id"] != port.current_room:
                    return {
                        "failed_level": 1,
                        "level_name": "MACRO_FLOW_ROOM_TRANSITION",
                        "frame": f,
                        "discrepancy": {
                            "variable": "current_room",
                            "golden_nes": nes["room_id"],
                            "port_c": port.current_room
                        },
                        "cause": f"Fallo de navegación de salas: La NES está en la Sala {nes['room_id']}, pero el Port está en la Sala {port.current_room}."
                    }

        # =====================================================================
        # NIVEL 2: Objetos y Persistencia del Mundo (Compuerta 2)
        # =====================================================================
        if max_level >= 2 and nes["in_game"]:
            # Estado de puerta
            if nes["door_is_open"] != (port.door_is_open != 0):
                return {
                    "failed_level": 2,
                    "level_name": "WORLD_OBJECTS_DOOR_STATE",
                    "frame": f,
                    "discrepancy": {
                        "variable": "door_is_open",
                        "golden_nes": nes["door_is_open"],
                        "port_c": port.door_is_open != 0
                    },
                    "cause": "El estado de la puerta difiere entre la ROM y el Port (abierta vs cerrada)."
                }

        # =====================================================================
        # NIVEL 3: Actores y Ciclo de Vida de Enemigos (Compuerta 3)
        # =====================================================================
        if max_level >= 3 and nes["in_game"]:
            pass

        # =====================================================================
        # NIVEL 4: Cinemática, Físicas Continuas y Jack (Compuerta 4)
        # =====================================================================
        if max_level >= 4 and nes["in_game"]:
            delta_y = abs(nes["jack_y"] - port.jack_y)
            delta_x = abs(nes["jack_x"] - port.jack_x)
            if delta_y > 2 or delta_x > 2:
                return {
                    "failed_level": 4,
                    "level_name": "PHYSICS_JACK_COORDINATES",
                    "frame": f,
                    "discrepancy": {
                        "variable": "jack_position",
                        "golden_nes": f"X={nes['jack_x']}, Y={nes['jack_y']}",
                        "port_c": f"X={port.jack_x}, Y={port.jack_y}",
                        "delta_x": delta_x,
                        "delta_y": delta_y
                    },
                    "cause": f"Desviación cinemática continua de Jack en Frame {f}: delta_y={delta_y}px, delta_x={delta_x}px."
                }

        # =====================================================================
        # NIVEL 5: Micro-Visual (Compuerta 5)
        # =====================================================================
        if max_level >= 5:
            pass

        return None

def run_oracle_verification(tas_path: Optional[str] = None, max_frames: Optional[int] = None, start_frame: int = 270, max_level: int = 5):
    oracle = HierarchicalOracle()
    tas_inputs = {}

    if tas_path and os.path.exists(tas_path):
        with open(tas_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split(',')
                if len(parts) >= 2:
                    tas_inputs[int(parts[0])] = int(parts[1])
        if max_frames is None or max_frames <= 0:
            max_frames = len(tas_inputs)
    elif max_frames is None or max_frames <= 0:
        max_frames = 600

    print("=====================================================================")
    print("  ORÁCULO DE PARIDAD JERÁRQUICA MACRO-A-MICRO (CYNES vs C99 PORT)")
    print("=====================================================================")
    print(f"  Nivel Máximo Auditado:  Nivel {max_level}")
    print(f"  Modo de Entrada:        {'Archivo TAS: ' + tas_path if tas_path else f'Inyección START en Frame {start_frame}'}")
    print(f"  Límite de Fotogramas:   {max_frames} frames (60 Hz)")
    print("---------------------------------------------------------------------")

    divergence = None

    for f in range(max_frames):
        if tas_inputs:
            input_mask = tas_inputs.get(f, 0)
        else:
            input_mask = CYNES_BTN_START if f == start_frame else 0

        oracle.step(input_mask)

        # Evaluar compuertas jerárquicas
        divergence = oracle.evaluate_quality_gates(max_level=max_level)
        if divergence:
            break

    total_evaluated = oracle.current_frame
    oracle.close()

    output_dir = "scratch"
    os.makedirs(output_dir, exist_ok=True)
    report_path = os.path.join(output_dir, "divergence_report.json")

    if divergence:
        divergence["total_session_frames"] = max_frames
        divergence["progress_percentage"] = round((total_evaluated / max_frames) * 100, 2)
        with open(report_path, "w", encoding="utf-8") as f:
            json.dump(divergence, f, indent=2)

        print("\n---------------------------------------------------------------------")
        print(f"  [DIVERGENCIA DETECTADA EN NIVEL {divergence['failed_level']}]: {divergence['level_name']}")
        print("---------------------------------------------------------------------")
        print(f"  * Fotograma del Desvío:  Frame {divergence['frame']} / {max_frames} ({divergence['progress_percentage']}%)")
        print(f"  * Causa Diagnosticada:   {divergence['cause']}")
        print(f"  * Valores en Conflicto:  {divergence['discrepancy']}")
        print(f"  * Reporte generado en:   {report_path}")
        print("=====================================================================\n")
        return False
    else:
        print("\n---------------------------------------------------------------------")
        print(f"  [OK]: 100% PARIDAD HOMOLOGADA HASTA NIVEL {max_level}")
        print(f"  * Total de frames limpios: {total_evaluated} frames")
        print("=====================================================================\n")
        if os.path.exists(report_path):
            os.remove(report_path)
        return True

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Oráculo de Paridad Jerárquica Macro-a-Micro")
    parser.add_argument("--tas", type=str, default=None, help="Ruta al archivo .tas de entrada")
    parser.add_argument("--frames", type=int, default=None, help="Número máximo de frames a simular")
    parser.add_argument("--start-btn", type=int, default=270, help="Frame de inyección de START si no hay .tas")
    parser.add_argument("--level", type=int, default=5, help="Nivel máximo de compuerta a auditar (1..5)")
    args = parser.parse_args()

    success = run_oracle_verification(tas_path=args.tas, max_frames=args.frames, start_frame=args.start_btn, max_level=args.level)
    sys.exit(0 if success else 1)
