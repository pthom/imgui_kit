#!/usr/bin/env python3
"""Copies the JavaScript files into the C++ sources that embed them.

The page's scripts live in .js files, where an editor and a linter read them. Each is embedded in a C++ source as a
raw string, between two marker lines:
    // <embedded file="<name>.js" sha1="<sha1 of the file>">
    ...
    // </embedded>
Run it after an edit to a .js file (from anywhere: the paths are the script's own). With --check, it changes nothing
and fails when a copy differs from its file: the CI runs it.
"""
import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EMBEDS = [("imgui_kit_emscripten_keyboard.cpp", "imgui_kit_emscripten_keyboard.js")]
MARKER = re.compile(r'^( *)// <embedded file="([^"]+)" sha1="([0-9a-f]*)">.*\n(.*?)^ *// </embedded>', re.M | re.S)


def embedded_block(indent: str, name: str, js: str) -> str:
    assert ')JS"' not in js, f"{name} holds the raw string's delimiter"
    sha1 = hashlib.sha1(js.encode()).hexdigest()
    return (f'{indent}// <embedded file="{name}" sha1="{sha1}">\n'
            f'{indent}const char* KEYBOARD_SCRIPT = R"JS(\n{js})JS";\n'
            f'{indent}// </embedded>')


def main() -> int:
    check = "--check" in sys.argv
    changed = []
    for cpp_name, js_name in EMBEDS:
        cpp_path, js_path = ROOT / cpp_name, ROOT / js_name
        cpp = cpp_path.read_text()
        js = js_path.read_text()
        m = MARKER.search(cpp)
        assert m is not None, f"{cpp_name}: no embedded block"
        assert m.group(2) == js_name, f"{cpp_name}: embeds {m.group(2)}, expected {js_name}"
        new = cpp[:m.start()] + embedded_block(m.group(1), js_name, js) + cpp[m.end():]
        if new != cpp:
            changed.append(cpp_name)
            if not check:
                cpp_path.write_text(new)
    if check and changed:
        print(f"out of date: {', '.join(changed)} (run tools/embed_js.py)")
        return 1
    print(f"updated: {', '.join(changed)}" if changed else "up to date")
    return 0


if __name__ == "__main__":
    sys.exit(main())
