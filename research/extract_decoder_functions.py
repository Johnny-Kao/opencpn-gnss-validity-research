#!/usr/bin/env python3
"""Generate isolated translation unit from exact production decoder methods.

The actual method bodies are extracted from comm_decoder.cpp unchanged.
Only their surrounding class, NavData and dependencies are supplied by a shim.
This is *not* a full CommBridge or GUI integration test.
"""
import argparse
from pathlib import Path

METHODS = ("DecodePGN129029", "DecodePGN129025")


def extract_method(source: str, name: str) -> str:
    signature = f"bool CommDecoder::{name}("
    start = source.find(signature)
    if start < 0:
        raise ValueError(f"method not found: {name}")
    opening = source.find("{", start)
    if opening < 0:
        raise ValueError(f"opening brace missing: {name}")
    depth = 0
    for i in range(opening, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise ValueError(f"unmatched braces: {name}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    data = Path(args.source).read_text()
    code = '#include "research/gnss_decoder_shim.hpp"\n\n'
    code += "\n\n".join(extract_method(data, m) for m in METHODS)
    Path(args.out).write_text(code + "\n")
    print("Extracted unmodified production methods:", ", ".join(METHODS))


if __name__ == "__main__":
    main()
