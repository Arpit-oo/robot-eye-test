from pathlib import Path
from SCons.Script import Import
import shutil

Import("env")

PROJECT_DIR = Path(env["PROJECT_DIR"])
DATA_DIR = PROJECT_DIR / "data"

SOURCE_GIFS = [
    (Path(r"C:\code\gettingshitdone\robot\animation\catgif.gif"), "catgif.gif"),
    (Path(r"C:\code\gettingshitdone\robot\animation\doggif1.gif"), "doggif1.gif"),
    (Path(r"C:\code\gettingshitdone\robot\animation\catsitt.gif"), "catsitt.gif"),
]

for src, _ in SOURCE_GIFS:
    if not src.exists():
        raise RuntimeError(f"Source GIF not found: {src}")

DATA_DIR.mkdir(parents=True, exist_ok=True)

for src, dest_name in SOURCE_GIFS:
    dest = DATA_DIR / dest_name
    shutil.copy2(src, dest)
    file_size = dest.stat().st_size
    print(f"[sync_gif] source: {src}")
    print(f"[sync_gif] dest: {dest}")
    print(f"[sync_gif] size: {file_size} bytes")
