#!/usr/bin/env python3
"""
tools/nes_oracle.py - Oráculo de Referencia Autónomo de NES (cynes)

Ejecuta la ROM original de Mighty Bomb Jack (REV_US) de forma headless mediante cynes
sin depender de binarios del source port en C. Sirve para:
1. Inspeccionar variables RAM ($0000-$07FF) y sprites OAM en micro-escenarios cortos.
2. Exportar fixtures JSON canónicos (--dump-state) para tests CTest (Capa 1) y
   micro-escenarios semánticos (Capa 2).
3. Capturar pantallas RGB de referencia (--capture-png) y proveer save_3panel_diff_png()
   reutilizable por tools/verify_scenarios.py.
"""

import os
import sys
import json
import argparse
import numpy as np
from typing import Dict, Any, List, Optional

if sys.stdout.encoding and sys.stdout.encoding.lower() != "utf-8":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

try:
    import cynes
except ImportError:
    print("[ERROR]: Se requiere la biblioteca 'cynes' de Python.")
    sys.exit(1)

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("[ERROR]: Se requiere 'Pillow' (PIL) para capturas y diffs PNG.")
    sys.exit(1)

BUTTON_MAP = {
    "RIGHT":  cynes.NES_INPUT_RIGHT,
    "LEFT":   cynes.NES_INPUT_LEFT,
    "DOWN":   cynes.NES_INPUT_DOWN,
    "UP":     cynes.NES_INPUT_UP,
    "START":  cynes.NES_INPUT_START,
    "SELECT": cynes.NES_INPUT_SELECT,
    "B":      cynes.NES_INPUT_B,
    "A":      cynes.NES_INPUT_A,
}


def bcd_byte_to_int(b: int) -> int:
    return ((b >> 4) & 0x0F) * 10 + (b & 0x0F)


def bcd4_le_to_int(score_bytes: List[int]) -> int:
    val = 0
    for i in range(3, -1, -1):
        val = val * 100 + bcd_byte_to_int(int(score_bytes[i]) & 0xFF)
    return val


def parse_input_script(script: Optional[str]) -> Dict[int, int]:
    """
    Parsea secuencias tipo '270:START@2,620:RIGHT+A@20' en un diccionario {frame: bitmask}.
    """
    timeline: Dict[int, int] = {}
    if not script:
        return timeline

    for token in script.split(","):
        tok = token.strip()
        if not tok:
            continue
        if ":" not in tok:
            raise ValueError(f"Segmento de script inválido '{tok}'; usa formato frame:BOTONES[@duracion].")
        frame_part, btn_part = tok.split(":", 1)
        start_f = int(frame_part.strip())
        duration = 1
        if "@" in btn_part:
            btn_str, dur_str = btn_part.split("@", 1)
            duration = max(1, int(dur_str.strip()))
        else:
            btn_str = btn_part

        mask = 0
        for name in btn_str.split("+"):
            key = name.strip().upper()
            if not key or key in ("NONE", "."):
                continue
            if key not in BUTTON_MAP:
                raise ValueError(f"Botón desconocido '{key}'. Válidos: {', '.join(BUTTON_MAP.keys())}")
            mask |= BUTTON_MAP[key]

        for f in range(start_f, start_f + duration):
            timeline[f] = timeline.get(f, 0) | mask

    return timeline


def parse_ram_addresses(spec: Optional[str]) -> List[int]:
    if not spec:
        return []
    addrs: List[int] = []
    for raw in spec.split(","):
        tok = raw.strip()
        if not tok:
            continue
        if tok.startswith("$"):
            addrs.append(int(tok[1:], 16) & 0x7FF)
        elif tok.lower().startswith("0x"):
            addrs.append(int(tok, 16) & 0x7FF)
        else:
            addrs.append(int(tok, 0) & 0x7FF)
    return addrs


def extract_oam_visible(nes: "cynes.NES") -> List[Dict[str, int]]:
    sprites = []
    for i in range(64):
        base = 0x200 + i * 4
        y = int(nes[base])
        if y >= 0xEF:
            continue
        tile = int(nes[base + 1])
        attr = int(nes[base + 2])
        x = int(nes[base + 3])
        sprites.append({
            "idx": i,
            "y": y,
            "tile": tile,
            "attr": attr,
            "x": x,
            "pal": attr & 0x03,
            "flip_h": 1 if (attr & 0x40) else 0,
            "flip_v": 1 if (attr & 0x80) else 0,
        })
    return sprites


def extract_canonical_state(nes: "cynes.NES", frame: int, watch_addrs: Optional[List[int]] = None) -> Dict[str, Any]:
    score_bcd = [int(nes[0x00A0 + i]) for i in range(4)]
    stage_timer_bcd = int(nes[0x00A4])
    ram_watch = {f"0x{a:04X}": int(nes[a]) for a in (watch_addrs or [])}
    return {
        "frame": frame,
        "in_game": int(nes[0x0043]),
        "game_state": int(nes[0x0044]),
        "title_state": int(nes[0x0045]),
        "score_bcd": score_bcd,
        "score_int": bcd4_le_to_int(score_bcd),
        "stage_timer_bcd": stage_timer_bcd,
        "stage_timer_dec": bcd_byte_to_int(stage_timer_bcd),
        "timer_frames": int(nes[0x00A5]),
        "alive_enemies": int(nes[0x00B1]),
        "room_id": int(nes[0x03BF]),
        "scroll_x": int(nes[0x03C2]),
        "jack_x": int(nes[0x03D8]),
        "jack_y": int(nes[0x03D9]),
        "jack_sub_x": int(nes[0x03DA]),
        "jack_sub_y": int(nes[0x03DB]),
        "mighty_level": int(nes[0x03E0]),
        "mighty_coins": int(nes[0x03E2]),
        "lives": int(nes[0x03E4]),
        "room_data_slot": [int(nes[0x0603 + i]) for i in range(16)],
        "oam_visible": extract_oam_visible(nes),
        "ram_watch": ram_watch,
    }


def save_3panel_diff_png(
    nes_rgb: np.ndarray,
    port_rgb: np.ndarray,
    title: str,
    out_path: str,
) -> Dict[str, float]:
    """
    Genera imagen PNG 3-paneles (NES Original | Port C11 | Pixel Diff) headless con Pillow.
    Acepta frames RGB888 de forma (240, 256, 3) uint8 y devuelve métricas de similitud.
    """
    width, height = 256, 240
    header_h = 32
    gap = 12
    canvas_w = width * 3 + gap * 2 + 16
    canvas_h = height + header_h + 12

    nes_u8 = np.asarray(nes_rgb, dtype=np.uint8).reshape((height, width, 3))
    port_u8 = np.asarray(port_rgb, dtype=np.uint8).reshape((height, width, 3))

    diff = np.abs(nes_u8.astype(np.int16) - port_u8.astype(np.int16)).max(axis=2)
    diff_mask = diff > 24
    all_pct = round(float(np.count_nonzero(~diff_mask)) / (width * height) * 100.0, 1)
    play_pct = round(float(np.count_nonzero(~diff_mask[32:, :])) / (width * 208) * 100.0, 1)

    diff_vis = (nes_u8 // 3).astype(np.uint8)
    diff_vis[diff_mask] = [255, 40, 40]

    canvas = Image.new("RGB", (canvas_w, canvas_h), (24, 24, 28))
    draw = ImageDraw.Draw(canvas)
    font = ImageFont.load_default()

    draw.text((8, 10), f"[1] NES ORIGINAL ({title})", fill=(76, 175, 80), font=font)
    draw.text((8 + width + gap, 10), "[2] C11 PORT", fill=(33, 150, 243), font=font)
    draw.text((8 + (width + gap) * 2, 10), f"[3] DIFF ALL:{all_pct}% PLAY:{play_pct}%", fill=(255, 152, 0), font=font)

    canvas.paste(Image.fromarray(nes_u8, mode="RGB"), (8, header_h))
    canvas.paste(Image.fromarray(port_u8, mode="RGB"), (8 + width + gap, header_h))
    canvas.paste(Image.fromarray(diff_vis, mode="RGB"), (8 + (width + gap) * 2, header_h))

    draw.rectangle([7, header_h - 1, 8 + width, header_h + height], outline=(76, 175, 80), width=1)
    draw.rectangle([7 + width + gap, header_h - 1, 8 + width * 2 + gap, header_h + height], outline=(33, 150, 243), width=1)
    draw.rectangle([7 + (width + gap) * 2, header_h - 1, 8 + width * 3 + gap * 2, header_h + height], outline=(255, 152, 0), width=1)

    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    canvas.save(out_path, format="PNG")
    return {"px_all_pct": all_pct, "px_play_pct": play_pct}


def format_mask_short(mask: int) -> str:
    if mask == 0:
        return "."
    labels = [
        (cynes.NES_INPUT_UP, "U"),
        (cynes.NES_INPUT_DOWN, "D"),
        (cynes.NES_INPUT_LEFT, "L"),
        (cynes.NES_INPUT_RIGHT, "R"),
        (cynes.NES_INPUT_A, "A"),
        (cynes.NES_INPUT_B, "B"),
        (cynes.NES_INPUT_START, "ST"),
        (cynes.NES_INPUT_SELECT, "SL"),
    ]
    return "+".join(lbl for bit, lbl in labels if mask & bit)


def main() -> None:
    parser = argparse.ArgumentParser(description="Oráculo autónomo de referencia NES (cynes)")
    parser.add_argument("--rom", type=str, default="asm/bin/mbj-us.nes", help="Ruta a la ROM NES USA")
    parser.add_argument("--frames", type=int, default=400, help="Total de frames a simular (default: 400)")
    parser.add_argument("--script", type=str, default=None, help="Secuencia frame:BOTONES[@duracion],...")
    parser.add_argument("--watch-ram", type=str, default=None, help="Direcciones RAM hex/dec separadas por coma")
    parser.add_argument("--at-frame", type=int, default=None, help="Frame objetivo para dumps/capturas (default: último)")
    parser.add_argument("--window", type=int, default=10, help="Ventana alrededor de --at-frame para watch RAM (default: 10)")
    parser.add_argument("--dump-oam", action="store_true", help="Imprimir sprites OAM visibles en --at-frame")
    parser.add_argument("--dump-state", type=str, default=None, help="Exportar fixture JSON canónico en --at-frame")
    parser.add_argument("--capture-png", type=str, default=None, help="Guardar PNG RGB 256x240 de cynes en --at-frame")
    args = parser.parse_args()

    if not os.path.exists(args.rom):
        print(f"[ERROR]: ROM no encontrada en '{args.rom}'.")
        sys.exit(1)

    total_frames = max(1, args.frames)
    target_frame = args.at_frame if args.at_frame is not None else (total_frames - 1)
    if target_frame < 0 or target_frame >= total_frames:
        print(f"[ERROR]: --at-frame {target_frame} fuera de rango [0, {total_frames - 1}].")
        sys.exit(1)

    timeline = parse_input_script(args.script)
    watch_addrs = parse_ram_addresses(args.watch_ram)

    nes = cynes.NES(args.rom)
    nes.reset()

    prev_watch: Optional[List[int]] = None
    watch_rows: List[str] = []
    target_state: Optional[Dict[str, Any]] = None
    target_pixels: Optional[np.ndarray] = None

    for f in range(total_frames):
        inp = timeline.get(f, 0)
        nes.controller = inp
        pixels = nes.step(1)

        if watch_addrs:
            cur_vals = [int(nes[a]) for a in watch_addrs]
            in_win = abs(f - target_frame) <= args.window
            changed = (prev_watch is None) or (cur_vals != prev_watch)
            if changed or in_win:
                mark = ">>" if f == target_frame else "  "
                vals_str = " ".join(f"0x{v:02X}" for v in cur_vals)
                watch_rows.append(f"{mark}{f:4d} | {format_mask_short(inp):^6} | {vals_str}")
            prev_watch = cur_vals

        if f == target_frame:
            target_state = extract_canonical_state(nes, f, watch_addrs)
            target_pixels = np.array(pixels, copy=True)

    if hasattr(nes, "close"):
        nes.close()
    del nes

    if watch_rows:
        hdr_addrs = " ".join(f"0x{a:04X}" for a in watch_addrs)
        print(f"\n=== WATCH RAM (total={total_frames}f, foco={target_frame}) ===")
        print(f"   FRM |  INP   | {hdr_addrs}")
        print("-" * (18 + len(hdr_addrs)))
        for row in watch_rows:
            print(row)

    if args.dump_oam and target_state is not None:
        oam = target_state["oam_visible"]
        print(f"\n=== NES OAM VISIBLE EN FRAME {target_frame} ({len(oam)} sprites) ===")
        print("IDX |   Y | TILE | ATTR |   X | PAL | FH | FV")
        print("-" * 46)
        for s in oam:
            print(
                f"{s['idx']:3d} | {s['y']:3d} | 0x{s['tile']:02X} | 0x{s['attr']:02X} | "
                f"{s['x']:3d} |  {s['pal']}  |  {s['flip_h']} |  {s['flip_v']}"
            )

    if args.dump_state and target_state is not None:
        os.makedirs(os.path.dirname(args.dump_state) or ".", exist_ok=True)
        with open(args.dump_state, "w", encoding="utf-8") as jf:
            json.dump(target_state, jf, indent=2)
        print(f"[OK]: Snapshot exportado en {args.dump_state} (Frame {target_frame}).")

    if args.capture_png and target_pixels is not None:
        os.makedirs(os.path.dirname(args.capture_png) or ".", exist_ok=True)
        img = Image.fromarray(target_pixels.reshape((240, 256, 3)), mode="RGB")
        img.save(args.capture_png, format="PNG")
        print(f"[OK]: Captura NES guardada en {args.capture_png} (Frame {target_frame}).")

    if not watch_rows and not args.dump_oam and not args.dump_state and not args.capture_png and target_state is not None:
        print(
            f"[NES Frame {target_frame}] IG={target_state['in_game']} ST={target_state['game_state']} "
            f"TS={target_state['title_state']} RM={target_state['room_id']} "
            f"JK=({target_state['jack_x']},{target_state['jack_y']}) "
            f"TM={target_state['stage_timer_dec']} LV={target_state['lives']} "
            f"EN={target_state['alive_enemies']} SC={target_state['score_int']}"
        )


if __name__ == "__main__":
    main()
