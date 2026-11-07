#!/usr/bin/env python3
"""Extract unchanged production CommBridge 129029/129025 handlers for a shim test."""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
out = ['#include "research/gnss_bridge_shim.hpp"']
for name in ("HandleN2K_129029", "HandleN2K_129025"):
    start = source.index(f"bool CommBridge::{name}(")
    begin = source.index("{", start)
    depth = 0
    for pos in range(begin, len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                out.append(source[start:pos+1])
                break
    else:
        raise RuntimeError("Unclosed method: " + name)
Path(sys.argv[2]).write_text("\n\n".join(out) + "\n")
print("Extracted two unchanged production CommBridge handlers")
