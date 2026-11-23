#!/usr/bin/env python3
"""
.agents/hooks/guard_workspace.py
Hook determinista PreToolUse para proteger el directorio inmutable asm/
y evitar la creación de scripts Python ad-hoc en scratch/.
"""

import json
import sys
from pathlib import Path, PurePath


def _resolve_target(raw_path: str, cwd: str, workspace_paths: list[str]) -> Path:
    p = Path(raw_path)
    if p.is_absolute():
        return p.resolve()
    base = Path(cwd) if cwd else (Path(workspace_paths[0]) if workspace_paths else Path.cwd())
    return (base / p).resolve()


def main() -> None:
    try:
        payload = json.load(sys.stdin)
    except Exception:
        print(json.dumps({"decision": "allow"}))
        return

    tool_call = payload.get("toolCall") or {}
    args = tool_call.get("args") or {}
    raw_path = args.get("Path") or args.get("path") or ""
    if not raw_path:
        print(json.dumps({"decision": "allow"}))
        return

    workspace_paths = payload.get("workspacePaths") or []
    cwd = payload.get("cwd") or ""
    target = _resolve_target(str(raw_path), str(cwd), workspace_paths)

    roots = [Path(__file__).resolve().parent.parent.parent]
    for wp in workspace_paths:
        try:
            roots.append(Path(wp).resolve())
        except Exception:
            pass

    for root in roots:
        try:
            rel = target.relative_to(root)
        except ValueError:
            continue

        parts = [part.lower() for part in PurePath(rel).parts]
        if not parts:
            continue

        if parts[0] == "asm":
            print(json.dumps({
                "decision": "deny",
                "reason": "Acceso denegado por workspace-guard: 'asm/' es un directorio inmutable de solo lectura.",
            }))
            return

        if parts[0] == "scratch" and target.suffix.lower() == ".py":
            print(json.dumps({
                "decision": "deny",
                "reason": (
                    "Scripts ad-hoc en scratch/*.py prohibidos. Usa tools/nes_oracle.py, "
                    "tools/asm_query.py o ctest."
                ),
            }))
            return

    print(json.dumps({"decision": "allow"}))


if __name__ == "__main__":
    main()
