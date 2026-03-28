from __future__ import annotations

import io
import json
import math
import os
import zipfile
from pathlib import Path
from typing import Any

from SCons.Script import Import  # type: ignore

Import("env")

PROJECT_DIR = Path(env["PROJECT_DIR"])
SOURCE_LOTTIE = Path(r"C:\code\gettingshitdone\robot\animation\Cat Movement.lottie")
OUTPUT_LOTTIE = SOURCE_LOTTIE.with_name("Cat_Movement_resized.lottie")
OUTPUT_HEADER = PROJECT_DIR / "include" / "cat_animation.h"

TARGET_W = 240
TARGET_H = 176


def _scale_xy(values: list[Any], scale: float, ox: float, oy: float) -> None:
    if len(values) >= 2 and isinstance(values[0], (int, float)) and isinstance(values[1], (int, float)):
        values[0] = float(values[0]) * scale + ox
        values[1] = float(values[1]) * scale + oy


def _scale_xy_no_offset(values: list[Any], scale: float) -> None:
    if len(values) >= 2 and isinstance(values[0], (int, float)) and isinstance(values[1], (int, float)):
        values[0] = float(values[0]) * scale
        values[1] = float(values[1]) * scale


def _scale_percent_xy(values: list[Any], scale: float) -> None:
    if len(values) >= 2 and isinstance(values[0], (int, float)) and isinstance(values[1], (int, float)):
        values[0] = float(values[0]) * scale
        values[1] = float(values[1]) * scale


def _apply_to_prop(prop: Any, fn) -> None:
    if not isinstance(prop, dict) or "k" not in prop:
        return

    k = prop["k"]
    if isinstance(k, list):
        if not k:
            return
        if isinstance(k[0], (int, float)):
            fn(k)
            return
        if isinstance(k[0], dict):
            for keyframe in k:
                if not isinstance(keyframe, dict):
                    continue
                for key in ("s", "e"):
                    v = keyframe.get(key)
                    if isinstance(v, list):
                        fn(v)
                for key in ("to", "ti"):
                    v = keyframe.get(key)
                    if isinstance(v, list):
                        _scale_xy_no_offset(v, SCALE)


def _scale_layer_ks(ks: dict[str, Any], scale: float, ox: float, oy: float, with_offset: bool) -> None:
    p = ks.get("p")
    if with_offset:
        _apply_to_prop(p, lambda arr: _scale_xy(arr, scale, ox, oy))
    else:
        _apply_to_prop(p, lambda arr: _scale_xy_no_offset(arr, scale))

    s = ks.get("s")
    _apply_to_prop(s, lambda arr: _scale_percent_xy(arr, scale))

    a = ks.get("a")
    _apply_to_prop(a, lambda arr: _scale_xy_no_offset(arr, scale))


def _scale_layers(layers: list[Any], scale: float, ox: float, oy: float, with_offset: bool) -> None:
    for layer in layers:
        if not isinstance(layer, dict):
            continue
        ks = layer.get("ks")
        if isinstance(ks, dict):
            _scale_layer_ks(ks, scale, ox, oy, with_offset)


def _find_animation_json_path(zf: zipfile.ZipFile) -> str:
    names = zf.namelist()

    if "manifest.json" in names:
        manifest = json.loads(zf.read("manifest.json").decode("utf-8"))
        animations = manifest.get("animations")
        if isinstance(animations, list) and animations:
            first = animations[0]
            if isinstance(first, dict):
                anim_id = first.get("id")
                if isinstance(anim_id, str) and anim_id:
                    candidate = f"animations/{anim_id}.json"
                    if candidate in names:
                        return candidate

    for name in names:
        if name.startswith("animations/") and name.endswith(".json"):
            return name

    raise RuntimeError("Could not find animation JSON inside .lottie")


def _pack_lottie(input_lottie: Path, output_lottie: Path, json_path: str, updated_json: str) -> None:
    with zipfile.ZipFile(input_lottie, "r") as zin, zipfile.ZipFile(output_lottie, "w", compression=zipfile.ZIP_DEFLATED) as zout:
        for name in zin.namelist():
            if name == json_path:
                zout.writestr(name, updated_json.encode("utf-8"))
            else:
                zout.writestr(name, zin.read(name))


def _write_header(json_text: str, header_path: Path) -> None:
    payload = json_text.encode("utf-8") + b"\0"
    header_path.parent.mkdir(parents=True, exist_ok=True)

    lines: list[str] = []
    chunk = 12
    for i in range(0, len(payload), chunk):
        part = payload[i : i + chunk]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in part) + ",")

    content = "\n".join(
        [
            "#ifndef CAT_ANIMATION_H",
            "#define CAT_ANIMATION_H",
            "",
            "#include <stdint.h>",
            "#include <stddef.h>",
            "",
            "// Generated from Cat_Movement_resized.lottie animation JSON.",
            "static const uint8_t cat_animation[] = {",
            *lines,
            "};",
            "",
            "static const size_t cat_animation_size = sizeof(cat_animation);",
            "",
            "#endif",
            "",
        ]
    )

    header_path.write_text(content, encoding="utf-8")


if not SOURCE_LOTTIE.exists():
    raise RuntimeError(f"Source .lottie file not found: {SOURCE_LOTTIE}")

with zipfile.ZipFile(SOURCE_LOTTIE, "r") as zf:
    json_path = _find_animation_json_path(zf)
    anim = json.loads(zf.read(json_path).decode("utf-8"))

orig_w = float(anim.get("w", 0))
orig_h = float(anim.get("h", 0))
if orig_w <= 0 or orig_h <= 0:
    raise RuntimeError("Animation JSON has invalid w/h")

SCALE = min(TARGET_W / orig_w, TARGET_H / orig_h)
scaled_w = orig_w * SCALE
scaled_h = orig_h * SCALE
offset_x = (TARGET_W - scaled_w) / 2.0
offset_y = (TARGET_H - scaled_h) / 2.0

anim["w"] = TARGET_W
anim["h"] = TARGET_H

layers = anim.get("layers")
if isinstance(layers, list):
    _scale_layers(layers, SCALE, offset_x, offset_y, with_offset=True)

assets = anim.get("assets")
if isinstance(assets, list):
    for asset in assets:
        if not isinstance(asset, dict):
            continue
        alayers = asset.get("layers")
        if isinstance(alayers, list):
            _scale_layers(alayers, SCALE, 0.0, 0.0, with_offset=False)

minified_json = json.dumps(anim, separators=(",", ":"), ensure_ascii=False)

_pack_lottie(SOURCE_LOTTIE, OUTPUT_LOTTIE, json_path, minified_json)
_write_header(minified_json, OUTPUT_HEADER)

print(f"[cat_resize] source: {SOURCE_LOTTIE}")
print(f"[cat_resize] resized: {OUTPUT_LOTTIE}")
print(f"[cat_resize] header: {OUTPUT_HEADER}")
print(f"[cat_resize] original: {int(orig_w)}x{int(orig_h)}")
print(f"[cat_resize] target: {TARGET_W}x{TARGET_H}")
print(f"[cat_resize] scale: {SCALE:.6f}")
print(f"[cat_resize] header bytes: {len(minified_json.encode('utf-8')) + 1}")
