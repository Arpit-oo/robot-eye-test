#!/usr/bin/env python3
"""Convert the 3 playlist GIFs into C header byte arrays.

Usage:
  python tools/convert_gifs_to_headers.py

The script writes headers into the project's include/ directory:
  - include/catsitt.h
  - include/doggif1.h
  - include/collarcatgif.h
"""

from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
INCLUDE_DIR = PROJECT_ROOT / "include"

GIF_SOURCES = [
    (Path(r"C:\code\gettingshitdone\robot\animation\catsitt.gif"), "catsitt"),
    (Path(r"C:\code\gettingshitdone\robot\animation\doggif1.gif"), "doggif1"),
    (Path(r"C:\code\gettingshitdone\robot\animation\collarcatgif.gif"), "collarcatgif"),
]

BYTES_PER_LINE = 12


def to_header_guard(name: str) -> str:
    return f"{name.upper()}_H"


def format_bytes(data: bytes) -> str:
    lines = []
    for i in range(0, len(data), BYTES_PER_LINE):
        chunk = data[i:i + BYTES_PER_LINE]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    if lines:
        lines[-1] = lines[-1].rstrip(",")
    return "\n".join(lines)


def write_header(src_path: Path, symbol: str) -> Path:
    if not src_path.exists():
        raise FileNotFoundError(f"Missing source GIF: {src_path}")

    data = src_path.read_bytes()
    header_path = INCLUDE_DIR / f"{symbol}.h"
    guard = to_header_guard(symbol)

    content = f"""#ifndef {guard}
#define {guard}

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

// Generated from: {src_path}
// Size: {len(data)} bytes
const uint8_t {symbol}_gif[] PROGMEM = {{
{format_bytes(data)}
}};

const size_t {symbol}_gif_len = sizeof({symbol}_gif);

#endif // {guard}
"""
    header_path.write_text(content, encoding="utf-8", newline="\n")
    return header_path


def main() -> None:
    INCLUDE_DIR.mkdir(parents=True, exist_ok=True)

    for src, symbol in GIF_SOURCES:
        out = write_header(src, symbol)
        print(f"[gif2h] {src} -> {out}")


if __name__ == "__main__":
    main()
