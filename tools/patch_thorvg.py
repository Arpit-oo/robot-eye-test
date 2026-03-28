from pathlib import Path

Import("env")


def patch_thorvg_lottie_property() -> None:
    project_dir = Path(env["PROJECT_DIR"])
    target = project_dir / ".pio" / "libdeps" / env["PIOENV"] / "lvgl" / "src" / "libs" / "thorvg" / "tvgLottieProperty.h"

    if not target.exists():
        print("[patch_thorvg] lvgl thorvg file not found yet; skipping")
        return

    old = "const_cast<LottieColorStop&>(other).value = {nullptr, nullptr};"
    new = "const_cast<LottieColorStop&>(other).value = ColorStop();"

    text = target.read_text(encoding="utf-8")
    if old in text:
        text = text.replace(old, new)
        target.write_text(text, encoding="utf-8")
        print("[patch_thorvg] patched tvgLottieProperty.h for GCC compatibility")
    else:
        print("[patch_thorvg] patch not needed")


patch_thorvg_lottie_property()
