#!/usr/bin/env python3
"""
tools/asm_query.py - Consultor Determinista de Bajo Coste para ASM 6502, RAM y Tablas

Consulta tools/asm_inventory.json, docs/nes-reference/inventories/data_inventory.csv y
docs/nes-reference/asm-to-c-map.md sin necesidad de explorar directorios ni hacer
búsquedas grep recursivas en todo el proyecto.
"""

import os
import sys
import csv
import json
import argparse
from pathlib import Path
from typing import Dict, Any, List, Optional

if sys.stdout.encoding and sys.stdout.encoding.lower() != 'utf-8':
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

ROOT = Path(__file__).resolve().parent.parent
INVENTORY_PATH = ROOT / "tools" / "asm_inventory.json"
DATA_CSV_PATH = ROOT / "docs" / "nes-reference" / "inventories" / "data_inventory.csv"
XREF_PATH = ROOT / "docs" / "nes-reference" / "asm-to-c-map.md"
ASM_SRC_DIR = ROOT / "asm" / "src"


def load_inventory() -> Dict[str, Any]:
    with open(INVENTORY_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def print_asm_snippet(filename: str, start_line: int, end_line: Optional[int], max_lines: int) -> None:
    target = ASM_SRC_DIR / filename
    if not target.exists():
        print(f"  [WARN]: Archivo ASM no encontrado: {target}")
        return
    lines = target.read_text(encoding="utf-8", errors="replace").splitlines()
    s = max(1, start_line)
    if end_line is None:
        e = min(len(lines), s + max_lines - 1)
    else:
        e = min(len(lines), end_line, s + max_lines - 1)
    print(f"  --- {filename}:{s}-{e} ---")
    for idx in range(s, e + 1):
        print(f"  {idx:5d}: {lines[idx - 1]}")
    if end_line and end_line > e:
        print(f"  ... (truncado a {max_lines} líneas; fin real en línea {end_line})")


def query_subroutine(inv: Dict[str, Any], term: str, snippet: bool, max_lines: int) -> int:
    q = term.lower()
    subs = inv.get("subroutines", [])
    exact = [s for s in subs if s["name"].lower() == q]
    matches = exact if exact else [s for s in subs if q in s["name"].lower()]
    if not matches:
        print(f"[NO MATCH]: Ninguna subrutina coincide con '{term}'.")
        return 1

    for s in matches[:10]:
        print(f"[SUB] {s['name']}  ({s['file']}:{s['start_line']}-{s['end_line']}, stack={s.get('stack_mutation', '?')})")
        if s.get("callers"):
            print(f"  callers : {', '.join(s['callers'][:15])}")
        if s.get("callees"):
            print(f"  callees : {', '.join(s['callees'][:15])}")
        if s.get("ram_r"):
            print(f"  ram_r   : {', '.join(s['ram_r'][:20])}")
        if s.get("ram_w"):
            print(f"  ram_w   : {', '.join(s['ram_w'][:20])}")
        if snippet:
            print_asm_snippet(s["file"], int(s["start_line"]), int(s["end_line"]), max_lines)
    if len(matches) > 10:
        print(f"... ({len(matches) - 10} coincidencias adicionales omitidas; afina el nombre)")
    return 0


def parse_hex_addr(term: str) -> Optional[int]:
    t = term.strip()
    if t.startswith("$"):
        t = "0x" + t[1:]
    if t.lower().startswith("0x"):
        try:
            return int(t, 16)
        except ValueError:
            return None
    if all(c in "0123456789abcdefABCDEF" for c in t) and len(t) <= 4 and any(c.isdigit() for c in t):
        try:
            return int(t, 16)
        except ValueError:
            return None
    return None


def query_ram(inv: Dict[str, Any], term: str) -> int:
    ram_vars = inv.get("ram_vars", [])
    subs = inv.get("subroutines", [])
    addr = parse_hex_addr(term)
    q = term.lower()

    if addr is not None:
        matches = [v for v in ram_vars if int(v.get("addr", -1)) == addr]
    else:
        exact = [v for v in ram_vars if v["name"].lower() == q]
        matches = exact if exact else [
            v for v in ram_vars if q in v["name"].lower() or q in v.get("comment", "").lower()
        ]

    if not matches:
        print(f"[NO MATCH]: Ninguna variable RAM coincide con '{term}'.")
        return 1

    for v in matches[:12]:
        vname = v["name"]
        readers = [f"{s['name']}({s['file']}:{s['start_line']})" for s in subs if vname in s.get("ram_r", [])]
        writers = [f"{s['name']}({s['file']}:{s['start_line']})" for s in subs if vname in s.get("ram_w", [])]
        comment = f" | {v['comment']}" if v.get("comment") else ""
        print(f"[RAM] {v['addr_hex']} {vname} (size={v.get('size', 1)}, ram.asm:{v.get('line', '?')}){comment}")
        print(f"  writers ({len(writers)}): {', '.join(writers[:15]) or '(ninguno detectado)'}")
        print(f"  readers ({len(readers)}): {', '.join(readers[:15]) or '(ninguno detectado)'}")
    if len(matches) > 12:
        print(f"... ({len(matches) - 12} coincidencias adicionales omitidas)")
    return 0


def query_table(term: str, snippet: bool, max_lines: int) -> int:
    if not DATA_CSV_PATH.exists():
        print(f"[ERROR]: No existe {DATA_CSV_PATH}")
        return 1
    q = term.lower()
    matches = []
    with open(DATA_CSV_PATH, "r", encoding="utf-8", errors="replace") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if q in row["table_name"].lower() or q in row.get("address", "").lower():
                matches.append(row)
    if not matches:
        print(f"[NO MATCH]: Ninguna tabla coincide con '{term}'.")
        return 1

    for row in matches[:15]:
        print(
            f"[TABLE] {row['table_name']} @ {row['address']} "
            f"({row['file']}:{row['line']}, size={row['size_bytes']}B, cat={row['category']})"
        )
        if snippet:
            print_asm_snippet(row["file"], int(row["line"]), None, min(max_lines, 40))
    if len(matches) > 15:
        print(f"... ({len(matches) - 15} tablas adicionales omitidas)")
    return 0


def query_jumptable(inv: Dict[str, Any], term: str) -> int:
    q = term.lower()
    jts = inv.get("jumptables", [])
    matches = [
        j for j in jts
        if q in j.get("parent_sub", "").lower()
        or q in j.get("selector", "").lower()
        or any(q in t.lower() for t in j.get("targets", []))
    ]
    if not matches:
        print(f"[NO MATCH]: Ninguna JumpTable coincide con '{term}'.")
        return 1

    for j in matches:
        print(f"[JT] {j['parent_sub']} ({j['file']}:{j['line']}) selector={j['selector']}")
        for idx, tgt in enumerate(j.get("targets", [])):
            print(f"  [{idx:2d}] {tgt}")
    return 0


def query_xref(term: str) -> int:
    if not XREF_PATH.exists():
        print(f"[ERROR]: No existe {XREF_PATH}")
        return 1
    q = term.lower()
    hits = []
    for idx, line in enumerate(XREF_PATH.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        if line.startswith("|") and q in line.lower():
            hits.append((idx, line))
    if not hits:
        print(f"[NO MATCH]: Ninguna entrada en CROSS_REFERENCE_INDEX.md coincide con '{term}'.")
        return 1
    for ln, text in hits[:25]:
        print(f"L{ln:3d}: {text}")
    return 0


def main() -> None:
    parser = argparse.ArgumentParser(description="Consultor determinista de símbolos 6502, RAM y tablas ROM")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--sub", type=str, help="Nombre o patrón de subrutina en asm_inventory.json")
    group.add_argument("--ram", type=str, help="Dirección hex ($00A4/0xA4) o nombre de variable RAM")
    group.add_argument("--table", type=str, help="Nombre o dirección de tabla en data_inventory.csv")
    group.add_argument("--jt", type=str, help="Nombre de rutina padre, selector o target de JumpTable")
    group.add_argument("--xref", type=str, help="Buscar término en docs/nes-reference/asm-to-c-map.md")
    parser.add_argument("--snippet", action="store_true", help="Mostrar líneas exactas de asm/src/")
    parser.add_argument("--max-lines", type=int, default=80, help="Máximo de líneas al imprimir --snippet (default: 80)")
    args = parser.parse_args()

    if args.sub:
        rc = query_subroutine(load_inventory(), args.sub, args.snippet, args.max_lines)
    elif args.ram:
        rc = query_ram(load_inventory(), args.ram)
    elif args.table:
        rc = query_table(args.table, args.snippet, args.max_lines)
    elif args.jt:
        rc = query_jumptable(load_inventory(), args.jt)
    else:
        rc = query_xref(args.xref)

    sys.exit(rc)


if __name__ == "__main__":
    main()
