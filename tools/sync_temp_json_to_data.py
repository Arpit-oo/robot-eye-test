import json
import re
from pathlib import Path

Import("env")

SOURCE = Path(r"C:\code\gettingshitdone\robot\animation\temp.json")


def load_json_text(src: Path) -> str:
    raw = src.read_text(encoding="utf-8").strip()
    if raw.startswith("{") or raw.startswith("["):
        obj = json.loads(raw)
        return json.dumps(obj, separators=(",", ":"))

    # Accept hex dump format like: 0x7B,0x22,... and decode to UTF-8 JSON text.
    hex_tokens = re.findall(r"0x[0-9a-fA-F]{2}", raw)
    if not hex_tokens:
        raise RuntimeError("temp.json is neither valid JSON text nor hex byte dump")

    data = bytes(int(tok, 16) for tok in hex_tokens)
    text = data.decode("utf-8", errors="strict").strip()
    obj = json.loads(text)
    return json.dumps(obj, separators=(",", ":"))


def main() -> None:
    if not SOURCE.exists():
        raise RuntimeError(f"Missing source file: {SOURCE}")

    project_dir = Path(env["PROJECT_DIR"])
    data_dir = project_dir / "data"
    data_dir.mkdir(parents=True, exist_ok=True)

    out = data_dir / "temp.json"
    json_text = load_json_text(SOURCE)
    out.write_text(json_text, encoding="utf-8")

    print(f"[sync_temp_json] source: {SOURCE}")
    print(f"[sync_temp_json] wrote: {out}")
    print(f"[sync_temp_json] bytes: {len(json_text.encode('utf-8'))}")


main()
