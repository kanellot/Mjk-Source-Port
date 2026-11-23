#!/usr/bin/env python3
"""
tools/inspect_parity.py - Inspector de Paridad en Ventana Temporal y Guardián Anti-Regresión

Herramienta de diagnóstico de Caja Negra / Telemetría Diferencial para agentes y desarrolladores.
Permite ejecutar sesiones TAS grabadas simultáneamente contra la ROM original (cynes) y el
Source Port en C99 (bin/mjk_core.dll), devolviendo:
1. Una ventana temporal de fotogramas [target - W, target + W] antes y después de la discrepancia.
2. Filtrado de desfases transitorios (--grace-frames): evita falsas alarmas cuando un cambio de
   chunk o estado ocurre con pocos frames de diferencia pero ambos motores convergen enseguida.
3. Exportación estructurada en CSV (scratch/window_report.csv) y JSON (scratch/window_report.json).
4. Captura visual comparativa Side-by-Side en PNG (scratch/diff_frame_XXXX.png).
5. Guardián Anti-Regresión y Anti-Bucle (scratch/oracle_history.json) que alerta si un cambio
   rompe fotogramas previamente superados o se estanca tras múltiples intentos.
"""

import os
import sys
import csv
import json
import ctypes
import argparse
import numpy as np
from collections import deque
from typing import Dict, Any, List, Optional, Tuple

# Forzar salida UTF-8 en consolas de Windows
if sys.stdout.encoding and sys.stdout.encoding.lower() != 'utf-8':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from hierarchical_parity_oracle import (
    HierarchicalOracle,
    CYNES_BTN_RIGHT, CYNES_BTN_LEFT, CYNES_BTN_DOWN, CYNES_BTN_UP,
    CYNES_BTN_START, CYNES_BTN_SELECT, CYNES_BTN_B, CYNES_BTN_A
)

def format_input_short(mask: int) -> str:
    """Devuelve representación compacta de los botones pulsados."""
    if mask == 0:
        return "."
    btns = []
    if mask & CYNES_BTN_UP:     btns.append("U")
    if mask & CYNES_BTN_DOWN:   btns.append("D")
    if mask & CYNES_BTN_LEFT:   btns.append("L")
    if mask & CYNES_BTN_RIGHT:  btns.append("R")
    if mask & CYNES_BTN_A:      btns.append("A")
    if mask & CYNES_BTN_B:      btns.append("B")
    if mask & CYNES_BTN_START:  btns.append("ST")
    if mask & CYNES_BTN_SELECT: btns.append("SL")
    return "+".join(btns)

def count_bits_24(b0: int, b1: int, b2: int) -> int:
    """Cuenta cuántos bits están a 1 en los 3 bytes de máscara de bombas."""
    return bin(b0 & 0xFF).count('1') + bin(b1 & 0xFF).count('1') + bin(b2 & 0xFF).count('1')

def compute_pixel_similarity(nes_pixels: np.ndarray, port_argb_bytes: bytes) -> float:
    """Calcula el porcentaje de píxeles visualmente coincidentes (256x240) entre NES y Port."""
    try:
        # nes_pixels: (240, 256, 3) en RGB
        nes_rgb = nes_pixels.reshape((240, 256, 3)).astype(np.int16)
        # port_argb_bytes: BGRA en little-endian (uint32 0xAARRGGBB)
        port_bgra = np.frombuffer(port_argb_bytes, dtype=np.uint8).reshape((240, 256, 4)).astype(np.int16)
        port_rgb = port_bgra[:, :, [2, 1, 0]]
        # Tolerancia de paleta NES (diferencia <= 24 por canal se considera mismo color visual)
        diff = np.abs(nes_rgb - port_rgb).max(axis=2)
        matching = np.count_nonzero(diff <= 24)
        return round((matching / (256 * 240)) * 100.0, 1)
    except Exception:
        return 0.0

def extract_rich_frame_record(oracle: HierarchicalOracle, input_mask: int, nes_pixels: np.ndarray, max_level: int = 4, pos_tol: int = 4) -> Dict[str, Any]:
    """Extrae un registro comparativo rico y semántico del fotograma actual, incluyendo similitud de píxeles."""
    nes_t = oracle.get_nes_telemetry()
    port = oracle.port_snap
    f = oracle.current_frame

    # Extraer framebuffer del Port para comparar píxeles
    fb_ptr = oracle.lib.MjkOracle_GetFramebuffer(oracle.oracle)
    port_argb_bytes = ctypes.string_at(fb_ptr, 256 * 240 * 4)
    px_match_pct = compute_pixel_similarity(nes_pixels, port_argb_bytes)

    # Conteo de bombas en RAM NES (bytes 4, 5, 6 del slot 0x603 -> 0x607..0x609)
    slot_nes = nes_t["room_slot"]
    nes_bombs_mask_bits = count_bits_24(slot_nes[4], slot_nes[5], slot_nes[6])
    nes_fire_bomb = slot_nes[3]

    # Conteo de bombas en RAM Port
    slot_port = [port.room_data_slot[i] for i in range(16)]
    port_bombs_mask_bits = count_bits_24(slot_port[4], slot_port[5], slot_port[6])
    port_fire_bomb = port.active_fire_bomb_index

    # Coordenada X en pantalla (OAM sprite de Jack en NES vs pantalla en Port)
    nes_screen_x = int(oracle.nes[0x203]) if nes_t["in_game"] else int(nes_t["jack_x"])

    # Evaluar discrepancia con tolerancia configurable para Nivel 4 (solo en Gameplay Activo: game_state == 5)
    div = oracle.evaluate_quality_gates(max_level=min(max_level, 3))
    if not div and max_level >= 4 and nes_t["in_game"] and nes_t["game_state"] == 5:
        dx = abs(nes_screen_x - port.jack_x)
        dy = abs(nes_t["jack_y"] - port.jack_y)
        if dx > pos_tol or dy > pos_tol:
            div = {
                "failed_level": 4,
                "level_name": "PHYSICS_JACK_COORDINATES",
                "frame": f,
                "discrepancy": {
                    "variable": "jack_screen_position",
                    "golden_nes": f"({nes_screen_x},{nes_t['jack_y']})",
                    "port_c": f"({port.jack_x},{port.jack_y})",
                    "delta_x": dx,
                    "delta_y": dy
                },
                "cause": f"Desviación espacial de Jack > {pos_tol}px: dx={dx}, dy={dy}"
            }

    status_str = "OK"
    if div:
        status_str = f"L{div['failed_level']}:{div['level_name']}"

    return {
        "frame": f,
        "input_mask": input_mask,
        "input_str": format_input_short(input_mask),
        "px_match_pct": px_match_pct,
        # NES Original
        "nes_in_game": int(nes_t["in_game"]),
        "nes_state": int(nes_t["game_state"]) if nes_t["in_game"] else int(nes_t["title_state"]),
        "nes_room": int(nes_t["room_id"]),
        "nes_door_open": int(nes_t["door_is_open"]),
        "nes_bombs_bits": nes_bombs_mask_bits,
        "nes_fire_idx": int(nes_fire_bomb),
        "nes_enemies": int(nes_t["active_enemies"]),
        "nes_jack_x": nes_screen_x,
        "nes_jack_local_x": int(nes_t["jack_x"]),
        "nes_jack_y": int(nes_t["jack_y"]),
        "nes_lives": int(nes_t["lives"]),
        # Port C99
        "port_in_game": int(port.in_game),
        "port_state": int(port.game_state) if port.in_game else int(port.title_screen_state),
        "port_room": int(port.current_room),
        "port_door_open": int(port.door_is_open != 0),
        "port_bombs_bits": port_bombs_mask_bits,
        "port_fire_idx": int(port_fire_bomb),
        "port_enemies": int(port.active_enemies_count),
        "port_jack_x": int(port.jack_x),
        "port_jack_y": int(port.jack_y),
        "port_lives": int(port.lives_count),
        # Estado de comparación
        "status": status_str,
        "divergence": div
    }

def save_side_by_side_png(nes_rgb_bytes: bytes, port_argb_bytes: bytes, frame_num: int, out_path: str, status_label: str = ""):
    """Genera una imagen PNG Triple (NES Original | C99 Port | Mapa de Diferencia de Píxeles) usando QtGui."""
    try:
        from PySide6 import QtCore, QtGui
        if not QtGui.QGuiApplication.instance():
            _app = QtGui.QGuiApplication([])

        width, height = 256, 240
        header_h = 32
        gap = 12
        canvas_w = width * 3 + gap * 2 + 16
        canvas_h = height + header_h + 12

        # Construir imagen de mapa de diferencias de píxeles (Rojo brillante donde difiere > 24, gris atenuado donde coincide)
        nes_rgb = np.frombuffer(nes_rgb_bytes, dtype=np.uint8).reshape((240, 256, 3)).astype(np.int16)
        port_bgra = np.frombuffer(port_argb_bytes, dtype=np.uint8).reshape((240, 256, 4)).astype(np.int16)
        port_rgb = port_bgra[:, :, [2, 1, 0]]
        diff_mask = np.abs(nes_rgb - port_rgb).max(axis=2) > 24
        match_pct = round((np.count_nonzero(~diff_mask) / (256 * 240)) * 100.0, 1)

        diff_vis = (nes_rgb // 3).astype(np.uint8)
        diff_vis[diff_mask] = [255, 40, 40]  # Rojo intenso en píxeles discrepantes
        diff_bytes = diff_vis.tobytes()

        canvas = QtGui.QImage(canvas_w, canvas_h, QtGui.QImage.Format_RGB32)
        canvas.fill(QtGui.QColor(24, 24, 28))

        painter = QtGui.QPainter(canvas)
        font = QtGui.QFont("Consolas", 9, QtGui.QFont.Bold)
        painter.setFont(font)

        # Cabeceras
        painter.setPen(QtGui.QColor(76, 175, 80))
        painter.drawText(8, 20, f"[1] NES ORIGINAL (Frame {frame_num})")

        painter.setPen(QtGui.QColor(33, 150, 243))
        painter.drawText(8 + width + gap, 20, f"[2] C99 PORT ({status_label or 'Inspect'})")

        painter.setPen(QtGui.QColor(255, 152, 0))
        painter.drawText(8 + (width + gap) * 2, 20, f"[3] PIXEL DIFF (Similitud: {match_pct}%)")

        # Imagen NES (RGB888)
        nes_img = QtGui.QImage(nes_rgb_bytes, width, height, width * 3, QtGui.QImage.Format_RGB888)
        painter.drawImage(8, header_h, nes_img)

        # Imagen Port (ARGB32)
        port_img = QtGui.QImage(port_argb_bytes, width, height, width * 4, QtGui.QImage.Format_ARGB32)
        painter.drawImage(8 + width + gap, header_h, port_img)

        # Imagen Diff (RGB888)
        diff_img = QtGui.QImage(diff_bytes, width, height, width * 3, QtGui.QImage.Format_RGB888)
        painter.drawImage(8 + (width + gap) * 2, header_h, diff_img)

        # Marcos
        painter.setPen(QtGui.QPen(QtGui.QColor(76, 175, 80), 2))
        painter.drawRect(7, header_h - 1, width + 1, height + 1)

        border_color = QtGui.QColor(244, 67, 54) if (status_label and status_label != "OK") else QtGui.QColor(33, 150, 243)
        painter.setPen(QtGui.QPen(border_color, 2))
        painter.drawRect(7 + width + gap, header_h - 1, width + 1, height + 1)

        painter.setPen(QtGui.QPen(QtGui.QColor(255, 152, 0), 2))
        painter.drawRect(7 + (width + gap) * 2, header_h - 1, width + 1, height + 1)

        painter.end()
        os.makedirs(os.path.dirname(out_path), exist_ok=True)
        canvas.save(out_path, "PNG")
        return True
    except Exception as e:
        print(f"[WARN]: No se pudo generar captura PNG Side-by-Side: {e}")
        return False

def evaluate_regression_guard(tas_key: str, level: int, current_fail_frame: Optional[int], total_frames: int, reset: bool = False) -> Dict[str, Any]:
    """
    Evalúa el Guardián Anti-Regresión y Anti-Bucle comparando el frame actual contra el historial.
    """
    os.makedirs("scratch", exist_ok=True)
    history_path = os.path.join("scratch", "oracle_history.json")
    history = {}
    if not reset and os.path.exists(history_path):
        try:
            with open(history_path, "r", encoding="utf-8") as f:
                history = json.load(f)
        except Exception:
            history = {}

    entry_key = f"{os.path.basename(tas_key)}:L{level}"
    prev = history.get(entry_key, {"best_frame": 0, "last_frame": 0, "stall_count": 0})

    effective_frame = total_frames if current_fail_frame is None else current_fail_frame
    best_prev = prev.get("best_frame", 0)
    stall_count = prev.get("stall_count", 0)

    verdict = "INITIAL"
    alert_msg = ""

    if reset or best_prev == 0:
        verdict = "BASELINE_SET"
        best_new = effective_frame
        stall_count = 0
        alert_msg = f"Línea base establecida en Frame {effective_frame}."
    elif effective_frame < best_prev:
        verdict = "CRITICAL_REGRESSION"
        best_new = best_prev
        stall_count += 1
        alert_msg = (
            f"[CRITICAL REGRESSION DETECTED]: El horizonte cayó del Frame {best_prev} al Frame {effective_frame} "
            f"(-{best_prev - effective_frame} frames). ¡REVERTIR CAMBIOS INMEDIATAMENTE (git checkout -- src/)!"
        )
    elif effective_frame == best_prev and current_fail_frame is not None:
        verdict = "STALLED"
        best_new = best_prev
        stall_count += 1
        if stall_count >= 3:
            verdict = "INFINITE_LOOP_ALERT"
            alert_msg = (
                f"[LOOP BLOQUEADO]: {stall_count} ejecuciones consecutivas atascadas en el Frame {effective_frame}. "
                f"PROHIBIDO seguir parcheando a ciegas el mismo punto. Revisa la ventana temporal y la imagen PNG."
            )
        else:
            alert_msg = f"Sin avance de horizonte (Frame {effective_frame}, intento #{stall_count})."
    else:
        verdict = "PROGRESS"
        best_new = effective_frame
        stall_count = 0
        gain = effective_frame - best_prev
        alert_msg = f"[PROGRESO CONFIRMADO]: Horizonte avanzó +{gain} frames ({best_prev} -> {effective_frame})."

    history[entry_key] = {
        "best_frame": best_new,
        "last_frame": effective_frame,
        "stall_count": stall_count,
        "verdict": verdict
    }
    with open(history_path, "w", encoding="utf-8") as f:
        json.dump(history, f, indent=2)

    return {
        "verdict": verdict,
        "best_frame": best_new,
        "current_frame": effective_frame,
        "stall_count": stall_count,
        "message": alert_msg
    }

def find_latest_tas() -> Optional[str]:
    tas_dir = "tests/tas"
    if not os.path.exists(tas_dir):
        return None
    files = [os.path.join(tas_dir, f) for f in os.listdir(tas_dir) if f.endswith(".tas")]
    if not files:
        return None
    return max(files, key=os.path.getmtime)

def load_tas_file(tas_path: str) -> Tuple[Dict[int, int], int]:
    tas_inputs = {}
    max_frame = 0
    with open(tas_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(",")
            if len(parts) >= 2:
                fr = int(parts[0])
                mask = int(parts[1])
                tas_inputs[fr] = mask
                if fr > max_frame:
                    max_frame = fr
    return tas_inputs, max_frame + 1

def print_window_table(records: List[Dict[str, Any]], target_frame: int):
    print("\n" + "=" * 120)
    print(f"  VENTANA TEMPORAL DE TELEMETRÍA Y PÍXELES (Foco en Frame {target_frame} | Total filas: {len(records)})")
    print("=" * 120)
    header = (
        f"{'FRM':>5} | {'INP':^5} | {'PX_SIM':^6} | "
        f"{'NES: IG/ST/RM/DR/BM/JK(X,Y)':^32} | "
        f"{'PORT: IG/ST/RM/DR/BM/JK(X,Y)':^32} | {'ESTADO':<22}"
    )
    print(header)
    print("-" * 120)
    for r in records:
        marker = ">>" if r["frame"] == target_frame else "  "
        px_col = f"{r.get('px_match_pct', 0.0):5.1f}%"
        nes_col = (
            f"{r['nes_in_game']}/{r['nes_state']:02d}/R{r['nes_room']:02d}/"
            f"D{r['nes_door_open']}/B{r['nes_bombs_bits']:02d}/"
            f"({r['nes_jack_x']:3d},{r['nes_jack_y']:3d})"
        )
        port_col = (
            f"{r['port_in_game']}/{r['port_state']:02d}/R{r['port_room']:02d}/"
            f"D{r['port_door_open']}/B{r['port_bombs_bits']:02d}/"
            f"({r['port_jack_x']:3d},{r['port_jack_y']:3d})"
        )
        print(f"{marker}{r['frame']:4d} | {r['input_str']:^5} | {px_col:^6} | {nes_col:^32} | {port_col:^32} | {r['status']:<22}")
    print("=" * 120)
    print("  Leyenda: PX_SIM=% Píxeles Iguales | IG=InGame | ST=State | RM=Room | DR=DoorOpen | BM=BombBits | JK=JackScreen(X,Y)\n")

def main():
    parser = argparse.ArgumentParser(description="Inspector de Paridad en Ventana Temporal y Guardián Anti-Regresión")
    parser.add_argument("--tas", type=str, default=None, help="Ruta al archivo .tas (por defecto usa el más reciente en tests/tas/)")
    parser.add_argument("--frame", type=int, default=None, help="Frame objetivo a inspeccionar (si se omite, busca automáticamente la 1ª divergencia persistente)")
    parser.add_argument("--window", type=int, default=15, help="Número de frames ANTES y DESPUÉS del frame objetivo (default: 15)")
    parser.add_argument("--grace-frames", type=int, default=8, help="Frames de gracia para ignorar desfases transitorios que se auto-resuelven (default: 8, 0=estricto)")
    parser.add_argument("--max-frames", type=int, default=None, help="Límite máximo de frames a simular")
    parser.add_argument("--level", type=int, default=2, help="Nivel máximo de compuerta a auditar (1:Macro, 2:Objetos, 3:Enemigos, 4:Cinemática)")
    parser.add_argument("--tolerance-px", type=int, default=6, help="Tolerancia en píxeles para posición de Jack en Nivel 4 (default: 6)")
    parser.add_argument("--visual", action="store_true", default=True, help="Generar captura PNG Side-by-Side del frame objetivo (activo por defecto)")
    parser.add_argument("--no-visual", action="store_false", dest="visual", help="Desactivar generación de captura PNG")
    parser.add_argument("--reset-history", action="store_true", help="Reiniciar el récord de horizonte en scratch/oracle_history.json")
    args = parser.parse_args()

    tas_path = args.tas or find_latest_tas()
    if not tas_path or not os.path.exists(tas_path):
        print("[ERROR]: No se encontró archivo TAS. Especifique --tas <ruta> o grabe uno con tools/dual_play_recorder.py.")
        sys.exit(1)

    tas_inputs, tas_total = load_tas_file(tas_path)
    sim_limit = args.max_frames if args.max_frames is not None else tas_total
    window = max(1, args.window)
    grace = max(0, args.grace_frames) if args.frame is None else 0

    oracle = HierarchicalOracle()
    history_ring = deque(maxlen=window + grace + 5)
    transient_shifts_ignored = []

    target_frame = args.frame
    found_divergence = None
    captured_nes_rgb = None
    captured_port_argb = None

    # Candidato pendiente de confirmar si dura más que grace_frames
    pending_candidate = None
    pending_nes_rgb = None
    pending_port_argb = None
    pending_streak = 0

    post_records = []
    capturing_post = False
    post_remaining = 0

    for f in range(sim_limit):
        inp = tas_inputs.get(f, 0)
        nes_pixels = oracle.step(inp)
        rec = extract_rich_frame_record(oracle, inp, nes_pixels, max_level=args.level, pos_tol=args.tolerance_px)

        if not capturing_post:
            history_ring.append(rec)

            # Modo 1: Frame manual fijo
            if args.frame is not None:
                if rec["frame"] == args.frame:
                    target_frame = rec["frame"]
                    found_divergence = rec["divergence"]
                    capturing_post = True
                    post_remaining = window
                    if args.visual:
                        captured_nes_rgb = bytes(nes_pixels.data)
                        fb_ptr = oracle.lib.MjkOracle_GetFramebuffer(oracle.oracle)
                        captured_port_argb = ctypes.string_at(fb_ptr, 256 * 240 * 4)
            # Modo 2: Búsqueda automática con filtro de gracia transitoria
            else:
                if rec["divergence"] is not None:
                    if pending_candidate is None:
                        pending_candidate = rec
                        pending_streak = 1
                        if args.visual:
                            pending_nes_rgb = bytes(nes_pixels.data)
                            fb_ptr = oracle.lib.MjkOracle_GetFramebuffer(oracle.oracle)
                            pending_port_argb = ctypes.string_at(fb_ptr, 256 * 240 * 4)
                    else:
                        pending_streak += 1

                    if pending_streak > grace:
                        # Divergencia persistente confirmada en pending_candidate["frame"]
                        target_frame = pending_candidate["frame"]
                        found_divergence = pending_candidate["divergence"]
                        captured_nes_rgb = pending_nes_rgb
                        captured_port_argb = pending_port_argb
                        capturing_post = True
                        # Ya hemos avanzado (pending_streak - 1) frames después del target_frame
                        already_after = pending_streak - 1
                        post_remaining = max(0, window - already_after)
                        if post_remaining == 0:
                            break
                else:
                    if pending_candidate is not None:
                        # ¡Se auto-resolvió dentro de la ventana de gracia!
                        transient_shifts_ignored.append({
                            "start_frame": pending_candidate["frame"],
                            "end_frame": rec["frame"] - 1,
                            "duration_frames": pending_streak,
                            "level_name": pending_candidate["divergence"]["level_name"]
                        })
                        pending_candidate = None
                        pending_streak = 0
                        pending_nes_rgb = None
                        pending_port_argb = None
        else:
            post_records.append(rec)
            post_remaining -= 1
            if post_remaining <= 0:
                break

    # Si terminó la simulación con un candidato aún pendiente, reportarlo
    if not capturing_post and pending_candidate is not None:
        target_frame = pending_candidate["frame"]
        found_divergence = pending_candidate["divergence"]
        captured_nes_rgb = pending_nes_rgb
        captured_port_argb = pending_port_argb

    total_simulated = oracle.current_frame
    oracle.close()

    if target_frame is None:
        target_frame = total_simulated

    # Construir la ventana exacta [target_frame - window, target_frame + window]
    combined = list(history_ring) + post_records
    all_window_records = [
        r for r in combined
        if (target_frame - window) <= r["frame"] <= (target_frame + window)
    ]

    # Evaluar Guardián Anti-Regresión
    reg_info = None
    if args.frame is None:
        fail_f = found_divergence["frame"] if found_divergence else None
        reg_info = evaluate_regression_guard(tas_path, args.level, fail_f, sim_limit, reset=args.reset_history)

    # Guardar CSV
    os.makedirs("scratch", exist_ok=True)
    csv_path = os.path.join("scratch", "window_report.csv")
    csv_fields = [
        "frame", "input_str", "px_match_pct",
        "nes_in_game", "nes_state", "nes_room", "nes_door_open", "nes_bombs_bits", "nes_fire_idx", "nes_enemies", "nes_jack_x", "nes_jack_y", "nes_lives",
        "port_in_game", "port_state", "port_room", "port_door_open", "port_bombs_bits", "port_fire_idx", "port_enemies", "port_jack_x", "port_jack_y", "port_lives",
        "status"
    ]
    with open(csv_path, "w", newline="", encoding="utf-8") as cf:
        writer = csv.DictWriter(cf, fieldnames=csv_fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(all_window_records)

    # Guardar captura visual Side-by-Side
    png_path = None
    if args.visual and captured_nes_rgb and captured_port_argb:
        png_path = os.path.join("scratch", f"diff_frame_{target_frame}.png")
        status_lbl = found_divergence["level_name"] if found_divergence else "OK"
        save_side_by_side_png(captured_nes_rgb, captured_port_argb, target_frame, png_path, status_lbl)

    # Guardar JSON
    json_path = os.path.join("scratch", "window_report.json")
    report_payload = {
        "tas_file": tas_path,
        "target_frame": target_frame,
        "total_tas_frames": sim_limit,
        "audited_level": args.level,
        "grace_frames": grace,
        "tolerance_px": args.tolerance_px,
        "transient_shifts_ignored": transient_shifts_ignored,
        "divergence": found_divergence,
        "regression_guard": reg_info,
        "visual_capture": png_path,
        "csv_report": csv_path,
        "window_records": all_window_records
    }
    with open(json_path, "w", encoding="utf-8") as jf:
        json.dump(report_payload, jf, indent=2)

    # Imprimir tabla de ventana y resumen ejecutivo
    print_window_table(all_window_records, target_frame)

    print("RESUMEN EJECUTIVO:")
    print(f"  * Archivo TAS:          {tas_path} ({sim_limit} frames)")
    if transient_shifts_ignored:
        print(f"  * Desfases Transitorios Ignorados (<= {grace}f): {len(transient_shifts_ignored)} -> {transient_shifts_ignored}")
    if found_divergence:
        print(f"  * Divergencia Real en:  Frame {found_divergence['frame']} [{found_divergence['level_name']}]")
        print(f"  * Causa:                {found_divergence['cause']}")
        print(f"  * Valores:              {found_divergence['discrepancy']}")
    else:
        print(f"  * Estado:               SIN DIVERGENCIAS PERSISTENTES hasta el frame {target_frame} (Nivel {args.level})")

    if reg_info:
        print(f"  * Guardián Regresión:   [{reg_info['verdict']}] {reg_info['message']}")
    if png_path and os.path.exists(png_path):
        print(f"  * Captura Side-by-Side: {png_path}")
    print(f"  * Reportes guardados:   {csv_path} | {json_path}\n")

    if reg_info and reg_info["verdict"] in ("CRITICAL_REGRESSION", "INFINITE_LOOP_ALERT"):
        sys.exit(2)
    sys.exit(1 if found_divergence else 0)

if __name__ == "__main__":
    main()
