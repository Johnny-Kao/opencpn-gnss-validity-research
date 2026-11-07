#!/usr/bin/env python3
"""Extract unmodified production CommBridge priority, watchdog, and PGN handlers.

This is method-level integration, not a full application build. Shims provide
wx/notification/message-bus dependencies while production control-flow and
state transitions are copied byte-for-byte from comm_bridge.cpp.
"""
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
target = Path(sys.argv[2])

def grab(signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise RuntimeError(f"Unclosed function: {signature}")

signatures = [
    "static string GetPriorityKey(",
    "static bool IsNextLowerPriorityAvailable(",
    "static void SelectNextLowerPriority(",
    "bool CommBridge::HandleN2K_129029(",
    "bool CommBridge::HandleN2K_129025(",
    "bool CommBridge::EvalPriority(",
    "void CommBridge::OnWatchdogTimer(",
]
out = ['#include "research/gnss_priority_bridge_shim.hpp"']
out.extend(grab(signature) for signature in signatures)
target.write_text("\n\n".join(out) + "\n")
print("Unchanged functions extracted:", len(signatures))
