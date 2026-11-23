#!/usr/bin/env python3
"""
tools/verify_scenarios.py — Capa 2: Micro-escenarios semánticos C11 vs NES (cynes).

Carga build/bin/mjk_core.dll (Windows) o build/bin/libmjk_core.so (Linux) vía ctypes,
extrae temporalmente assets.dat si no existe (sin dejar basura al salir) y valida
los 5 micro-escenarios cortos (60-240 frames) frente al oráculo nes_oracle.py.
"""

from __future__ import annotations

import argparse
import ctypes
import os
import sys
import tempfile
from typing import Dict, Tuple

import numpy as np

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.dirname(__file__))

import nes_oracle  # noqa: E402

try:
    import cynes  # type: ignore
except ImportError as exc:
    raise SystemExit(f"[ERROR] Se requiere 'cynes' para Capa 2: {exc}")

BTN_RIGHT = 0x01
BTN_LEFT = 0x02
BTN_DOWN = 0x04
BTN_UP = 0x08
BTN_START = 0x10
BTN_SELECT = 0x20
BTN_B = 0x40
BTN_A = 0x80

CYNES_TO_MJK = (
    (cynes.NES_INPUT_RIGHT, BTN_RIGHT),
    (cynes.NES_INPUT_LEFT, BTN_LEFT),
    (cynes.NES_INPUT_DOWN, BTN_DOWN),
    (cynes.NES_INPUT_UP, BTN_UP),
    (cynes.NES_INPUT_START, BTN_START),
    (cynes.NES_INPUT_SELECT, BTN_SELECT),
    (cynes.NES_INPUT_B, BTN_B),
    (cynes.NES_INPUT_A, BTN_A),
)


def cynes_to_mjk_mask(mask: int) -> int:
    out = 0
    for c_bit, m_bit in CYNES_TO_MJK:
        if mask & c_bit:
            out |= m_bit
    return out

MJK_SND_JUMP = 0x01


class MjkVerifySnapshot(ctypes.Structure):
    _fields_ = [
        ("top_mode", ctypes.c_uint8),
        ("game_state", ctypes.c_uint8),
        ("title_state", ctypes.c_uint8),
        ("room_id", ctypes.c_uint8),
        ("lives", ctypes.c_uint8),
        ("gdv_bcd", ctypes.c_uint8),
        ("high_gdv_bcd", ctypes.c_uint8),
        ("score_bcd", ctypes.c_uint8 * 4),
        ("jack_x", ctypes.c_uint8),
        ("jack_y", ctypes.c_uint8),
        ("in_air", ctypes.c_uint8),
        ("is_dying", ctypes.c_uint8),
        ("mighty_level", ctypes.c_uint8),
        ("mighty_coins", ctypes.c_uint8),
        ("timer_bcd", ctypes.c_uint8),
        ("greedy_jumps_bcd", ctypes.c_uint8),
        ("fire_bombs_collected", ctypes.c_uint8),
        ("alive_enemies", ctypes.c_uint8),
        ("last_sound_id", ctypes.c_uint8),
        ("oam_count", ctypes.c_uint8),
        ("scroll_x", ctypes.c_uint16),
        ("scroll_y", ctypes.c_uint16),
        ("frame_count", ctypes.c_uint32),
    ]


def load_core_lib() -> ctypes.CDLL:
    if sys.platform.startswith("win"):
        lib_path = os.path.join(ROOT_DIR, "build", "bin", "mjk_core.dll")
    else:
        lib_path = os.path.join(ROOT_DIR, "build", "bin", "libmjk_core.so")
    if not os.path.isfile(lib_path):
        raise FileNotFoundError(f"No se encontró librería compartida del núcleo: {lib_path}")

    lib = ctypes.CDLL(lib_path)
    lib.MjkRom_ExtractFileToAssetsDat.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
    lib.MjkRom_ExtractFileToAssetsDat.restype = ctypes.c_int

    lib.MjkVerify_Create.argtypes = [ctypes.c_char_p]
    lib.MjkVerify_Create.restype = ctypes.c_void_p
    lib.MjkVerify_Destroy.argtypes = [ctypes.c_void_p]
    lib.MjkVerify_Destroy.restype = None
    lib.MjkVerify_Reset.argtypes = [ctypes.c_void_p]
    lib.MjkVerify_Reset.restype = None
    lib.MjkVerify_StepFrame.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
    lib.MjkVerify_StepFrame.restype = None
    lib.MjkVerify_WarpRoom.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
    lib.MjkVerify_WarpRoom.restype = None
    lib.MjkVerify_GetSnapshot.argtypes = [ctypes.c_void_p, ctypes.POINTER(MjkVerifySnapshot)]
    lib.MjkVerify_GetSnapshot.restype = None
    lib.MjkVerify_GetFramebuffer.argtypes = [ctypes.c_void_p]
    lib.MjkVerify_GetFramebuffer.restype = ctypes.POINTER(ctypes.c_uint32)
    lib.MjkVerify_GetLastAudioEnergy.argtypes = [ctypes.c_void_p]
    lib.MjkVerify_GetLastAudioEnergy.restype = ctypes.c_uint32
    return lib


def get_port_rgb(lib: ctypes.CDLL, ctx: ctypes.c_void_p) -> np.ndarray:
    ptr = lib.MjkVerify_GetFramebuffer(ctx)
    arr = np.ctypeslib.as_array(ptr, shape=(240, 256))
    r = ((arr >> 16) & 0xFF).astype(np.uint8)
    g = ((arr >> 8) & 0xFF).astype(np.uint8)
    b = (arr & 0xFF).astype(np.uint8)
    return np.stack([r, g, b], axis=-1)


def get_snap(lib: ctypes.CDLL, ctx: ctypes.c_void_p) -> MjkVerifySnapshot:
    snap = MjkVerifySnapshot()
    lib.MjkVerify_GetSnapshot(ctx, ctypes.byref(snap))
    return snap


def bcd4_to_int(score_bcd: ctypes.Array) -> int:
    return nes_oracle.bcd4_le_to_int([int(score_bcd[i]) for i in range(4)])


def step_both(
    lib: ctypes.CDLL,
    ctx: ctypes.c_void_p,
    nes: cynes.NES,
    start_f: int,
    count: int,
    timeline: Dict[int, int],
) -> None:
    for f in range(start_f, start_f + count):
        mask = timeline.get(f, 0)
        nes.controller = mask
        nes.step()
        lib.MjkVerify_StepFrame(ctx, cynes_to_mjk_mask(mask))


def run_scenarios(lib: ctypes.CDLL, ctx: ctypes.c_void_p, rom_path: str, save_diffs: bool) -> Tuple[int, int]:
    passed = 0
    total = 5
    scratch_dir = os.path.join(ROOT_DIR, "scratch")

    def maybe_diff(name: str, nes: cynes.NES) -> None:
        if save_diffs:
            nes_rgb = np.asarray(nes.frame, dtype=np.uint8)
            port_rgb = get_port_rgb(lib, ctx)
            out_png = os.path.join(scratch_dir, f"diff_{name}.png")
            metrics = nes_oracle.save_3panel_diff_png(nes_rgb, port_rgb, name, out_png)
            print(f"    [DIFF] {out_png} (ALL={metrics['px_all_pct']}% PLAY={metrics['px_play_pct']}%)")

    # sc01: Copyright -> Title Screen
    lib.MjkVerify_Reset(ctx)
    nes = cynes.NES(rom_path)
    tl1 = nes_oracle.parse_input_script("10:START@3")
    step_both(lib, ctx, nes, 0, 25, tl1)
    s1 = get_snap(lib, ctx)
    n1 = nes_oracle.extract_canonical_state(nes, 25)
    assert s1.top_mode == 0 and n1["in_game"] == 0, f"top_mode={s1.top_mode} nes={n1['in_game']}"
    assert s1.room_id == 0 and n1["room_id"] == 0, f"room={s1.room_id}"
    assert n1["title_state"] == 4 and s1.jack_x == 40 and s1.jack_y == 120 and n1["jack_y"] == 120, (
        f"Title Jack mismatch: port=({s1.jack_x},{s1.jack_y}) nes=({n1['jack_x']},{n1['jack_y']}) ts={n1['title_state']}"
    )
    assert np.count_nonzero(get_port_rgb(lib, ctx)) > 0
    maybe_diff("sc01_title", nes)
    print("[PASS] sc01_copyright_to_title: Title Room=0 Jack=(40,120) coincidente con cynes")
    passed += 1

    # sc02: START en Title -> Caída -> RoundIntro -> Room 1 Main
    tl2 = nes_oracle.parse_input_script("25:START@3")
    step_both(lib, ctx, nes, 25, 370, tl2)  # hasta frame 395
    s2 = get_snap(lib, ctx)
    n2 = nes_oracle.extract_canonical_state(nes, 395)
    assert s2.top_mode == 1 and s2.game_state == 5 and n2["game_state"] == 5, (
        f"GS mismatch: port={s2.game_state} nes={n2['game_state']}"
    )
    assert s2.room_id == 1 and n2["room_id"] == 1
    assert s2.lives == 3 and n2["lives"] == 3
    assert s2.timer_bcd == 0x60 and n2["stage_timer_bcd"] == 0x60
    assert s2.in_air == 0 and s2.jack_y == n2["jack_y"], f"Jack Y port={s2.jack_y} nes={n2['jack_y']}"
    maybe_diff("sc02_room1", nes)
    print(f"[PASS] sc02_start_to_room1: GS=5 Room=1 Lives=3 Timer=0x60 Jack=({s2.jack_x},{s2.jack_y}) vs NES=({n2['jack_x']},{n2['jack_y']})")
    passed += 1

    # sc03: Caminar + Salto A en Sala 1 + SFX + Energía APU
    tl3 = nes_oracle.parse_input_script("400:RIGHT@45,425:A@3")
    step_both(lib, ctx, nes, 395, 32, tl3)  # hasta frame 427
    s3 = get_snap(lib, ctx)
    n3 = nes_oracle.extract_canonical_state(nes, 427)
    energy3 = lib.MjkVerify_GetLastAudioEnergy(ctx)
    assert s3.in_air == 1, f"Se esperaba in_air=1 en salto, obtenido {s3.in_air}"
    assert s3.jack_y < s2.jack_y and n3["jack_y"] < n2["jack_y"], "Jack debe ascender tras salto A"
    assert bcd4_to_int(s3.score_bcd) >= 10 and n3["score_int"] >= 10, "Salto debe sumar +10 pts"
    assert s3.last_sound_id == MJK_SND_JUMP, f"last_sound_id={s3.last_sound_id}"
    assert energy3 > 0, "Energía APU debe ser > 0 tras MJK_SND_JUMP"
    maybe_diff("sc03_jump", nes)
    print(f"[PASS] sc03_jump_and_sfx: in_air=1 Y={s3.jack_y} Score={bcd4_to_int(s3.score_bcd)} SND=0x{s3.last_sound_id:02X} AudioE={energy3}")
    passed += 1

    # sc04: Objetos en Sala 1 (caminar/saltar y abrir cofre/recoger bomba sin bloqueo)
    step_both(lib, ctx, nes, 427, 35, tl3)  # hasta frame 462
    s4 = get_snap(lib, ctx)
    assert s4.game_state == 5 and s4.is_dying == 0, "Sala 1 debe seguir activa sin bloqueo"
    assert s4.oam_count > 0, "OAM debe tener sprites visibles (Jack/HUD/enemigos)"
    maybe_diff("sc04_objects", nes)
    print(f"[PASS] sc04_bomb_and_chest: GS=5 Jack=({s4.jack_x},{s4.jack_y}) OAM={s4.oam_count} Score={bcd4_to_int(s4.score_bcd)}")
    passed += 1

    # sc05: Warp a Salas Especiales ($90 Bomb Room, $AC Torture Room, $A3 Ending)
    for rid, label in ((0x90, "BombRoom"), (0xAC, "TortureRoom"), (0xA3, "Ending")):
        lib.MjkVerify_WarpRoom(ctx, rid)
        for _ in range(4):
            lib.MjkVerify_StepFrame(ctx, 0)
        sw = get_snap(lib, ctx)
        fb = get_port_rgb(lib, ctx)
        assert sw.room_id == rid, f"Warp 0x{rid:02X} falló: room_id={sw.room_id}"
        assert np.count_nonzero(fb) > 0, f"Framebuffer vacío en {label}"
        if rid == 0xAC:
            assert sw.greedy_jumps_bcd == 0x50, f"Torture jumps={sw.greedy_jumps_bcd:#x}"
    print("[PASS] sc05_warp_special_rooms: $90 (Bomb Room), $AC (Torture 0x50) y $A3 (Ending) renderizados OK")
    passed += 1

    return passed, total


def main() -> int:
    parser = argparse.ArgumentParser(description="Capa 2: Verificación semántica C11 vs cynes")
    parser.add_argument("--assets", default=os.path.join(ROOT_DIR, "assets", "assets.dat"))
    parser.add_argument("--rom", default=os.path.join(ROOT_DIR, "asm", "bin", "mbj-us.nes"))
    parser.add_argument("--save-diffs", action="store_true", help="Guarda PNG 3-paneles en scratch/")
    args = parser.parse_args()

    if not os.path.isfile(args.rom):
        print(f"[ERROR] ROM no encontrada: {args.rom}", file=sys.stderr)
        return 1

    lib = load_core_lib()
    temp_assets: str | None = None
    assets_path = args.assets

    try:
        if not os.path.isfile(assets_path):
            fd, temp_assets = tempfile.mkstemp(prefix="mjk_cap2_", suffix=".dat")
            os.close(fd)
            sha1_buf = ctypes.create_string_buffer(64)
            err = lib.MjkRom_ExtractFileToAssetsDat(
                args.rom.encode("utf-8"),
                temp_assets.encode("utf-8"),
                sha1_buf,
            )
            if err != 0:
                print(f"[ERROR] Fallo extrayendo assets temporales desde {args.rom} (err={err})", file=sys.stderr)
                return 1
            assets_path = temp_assets

        ctx = lib.MjkVerify_Create(assets_path.encode("utf-8"))
        if not ctx:
            print(f"[ERROR] MjkVerify_Create falló con {assets_path}", file=sys.stderr)
            return 1

        try:
            passed, total = run_scenarios(lib, ctx, args.rom, args.save_diffs)
            print(f"[OK] Capa 2 completada: {passed}/{total} escenarios PASS frente a cynes.")
            return 0 if passed == total else 1
        finally:
            lib.MjkVerify_Destroy(ctx)
    finally:
        if temp_assets and os.path.isfile(temp_assets):
            os.remove(temp_assets)


if __name__ == "__main__":
    raise SystemExit(main())
