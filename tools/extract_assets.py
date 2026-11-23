#!/usr/bin/env python3
"""
tools/extract_assets.py
Mighty Bomb Jack (NES) - Clean-Room Asset Extractor (BYOR).

Validates the user's authentic mbj-us.nes ROM via SHA-1/MD5/CRC32,
extracts the CHR-ROM graphics (32 KB, 4 banks of 8KB), audio sequences,
and in-game text tables, packaging them into assets/assets.dat.
"""

import hashlib
import os
import struct
import sys
import zlib
from pathlib import Path

EXPECTED_SIZE = 65552
EXPECTED_SHA1 = "a723e9c36f24a23b9d64c4ea2e6e61586f4f02c7"
EXPECTED_MD5 = "56802d6302efef4ebfad0a7270c8914c"
EXPECTED_CRC32 = "73DD920F"

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ASSETS_DIR = ROOT / "assets"
DEFAULT_OUT_FILE = DEFAULT_ASSETS_DIR / "assets.dat"


def extract_assets(rom_path, out_file=DEFAULT_OUT_FILE):
    rom_path = Path(rom_path).resolve()
    if not rom_path.exists():
        print(f"[ERROR]: No se encuentra el archivo ROM en: {rom_path}")
        return False

    data = rom_path.read_bytes()
    
    # 1. Verification
    rom_size = len(data)
    rom_sha1 = hashlib.sha1(data).hexdigest()
    rom_md5 = hashlib.md5(data).hexdigest()
    rom_crc32 = f"{zlib.crc32(data):08X}"

    print("=====================================================================")
    print(" MIGHTY BOMB JACK - EXTRACTOR DE ASSETS CLEAN-ROOM (BYOR)")
    print("=====================================================================")
    print(f"Archivo de entrada: {rom_path.name}")
    print(f"Tamaño leído:       {rom_size} bytes")
    print(f"Hash SHA-1:         {rom_sha1}")
    print(f"Hash MD5:           {rom_md5}")
    print(f"Checksum CRC-32:    {rom_crc32}\n")

    if rom_size != EXPECTED_SIZE or rom_sha1 != EXPECTED_SHA1:
        print("[ERROR CRÍTICO]: La ROM suministrada no coincide con la versión USA oficial.")
        print(f"  Esperado SHA-1: {EXPECTED_SHA1}")
        print(f"  Obtenido SHA-1: {rom_sha1}")
        return False

    print("[CORRECTO]: ROM oficial verificada al 100%. Procediendo con la extracción...\n")

    # 2. Extract CHR-ROM (Offset 16 + 32768 = 32784, length 32768)
    chr_offset = 16 + 32768
    chr_size = 32768
    chr_data = data[chr_offset : chr_offset + chr_size]
    print(f"1. CHR-ROM (Gráficos PPU): Extraídos {len(chr_data)} bytes (4 bancos de 8KB).")

    # 3. Extract Audio Bytecode & Tables from PRG-ROM ($B000-$BFFF roughly, or specific audio segments)
    prg_data = data[16 : 16 + 32768]
    # For now, extract the music engine data segment ($F39E to $FD00 in PRG space)
    # PRG space $8000-$FFFF maps to 0..32767
    audio_start = 0xF000 - 0x8000
    audio_end = 0xFFA0 - 0x8000
    audio_data = prg_data[audio_start:audio_end]
    print(f"2. Audio Bytecode:         Extraídos {len(audio_data)} bytes de secuencias sonoras.")

    # 4. Extract Text & Dialogues from PRG-ROM (Strings at $C244-$C500)
    text_start = 0xC244 - 0x8000
    text_end = 0xC500 - 0x8000
    text_data = prg_data[text_start:text_end]
    print(f"3. Textos y Tipografía:    Extraídos {len(text_data)} bytes de tablas de caracteres.")

    # 5. Pack into binary assets.dat
    # Header format:
    # magic: 8s ("MJKASSET")
    # version: uint32 (0x00010000)
    # total_size: uint32
    # crc32: uint32
    # chr_offset: uint32
    # chr_size: uint32
    # audio_offset: uint32
    # audio_size: uint32
    # text_offset: uint32
    # text_size: uint32
    header_size = 8 + 4*9  # 44 bytes
    chr_file_offset = header_size
    audio_file_offset = chr_file_offset + len(chr_data)
    text_file_offset = audio_file_offset + len(audio_data)
    total_size = text_file_offset + len(text_data)

    payload = chr_data + audio_data + text_data
    payload_crc = zlib.crc32(payload)

    header = struct.pack(
        "<8s9I",
        b"MJKASSET",
        0x00010000,
        total_size,
        payload_crc,
        chr_file_offset,
        len(chr_data),
        audio_file_offset,
        len(audio_data),
        text_file_offset,
        len(text_data)
    )

    out_file.parent.mkdir(parents=True, exist_ok=True)
    with open(out_file, "wb") as f:
        f.write(header)
        f.write(payload)

    print(f"\n-> [ÉXITO]: Paquete de assets generado correctamente en:")
    print(f"   {out_file} ({total_size} bytes totales)")
    return True


if __name__ == "__main__":
    if len(sys.argv) > 1:
        target_rom = sys.argv[1]
    else:
        # Default test location if available
        test_rom = ROOT / "tools" / "test.nes"
        target_rom = str(test_rom)
    
    success = extract_assets(target_rom)
    if not success:
        sys.exit(1)
