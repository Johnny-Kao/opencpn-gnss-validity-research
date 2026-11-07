# GNSS validity research — verified results (2026-11-07)

Repository: https://github.com/Johnny-Kao/opencpn-gnss-validity-research

Working branch: `research/gnss-method-validation`.
Public main remains a clean, independently copied, sanitized upstream snapshot.

## Evidence

- Native vendored N2KParser: **PASS** with ASan/UBSan for GNSS methods 0, 1–5, 14, 15.
  Run: https://github.com/Johnny-Kao/opencpn-gnss-validity-research/actions/runs/42342246660
- Exact production `CommDecoder::DecodePGN129029` method extracted without changing its body:
  baseline from unmodified `main` **FAIL** for 0/14/15; candidate on research
  branch **PASS** for 0/1–5/14/15.
- Exact production `CommBridge::HandleN2K_129029` and
  `CommBridge::HandleN2K_129025` methods compiled with a focused shim:
  no-fix 129029 is rejected by patched decoder, but 129025 from the same
  constructed N2K source still updates position and refreshes watchdog.
  `EvalPriority` is **stubbed to accept**. The bypass test does not establish
  actual priority failover behavior.
- Comparison + handler replay CI: **PASS**.
  https://github.com/Johnny-Kao/opencpn-gnss-validity-research/actions/runs/42343465819

## Scope and limitations

The six-line candidate fix in `model/src/comm_decoder.cpp` is **partial**.
It prevents explicitly invalid 129029 methods from feeding the position
watchdog. It does NOT track fix quality across 129025 and 129029 from the
same physical source. A full fix would require per-source state semantics,
expiry/reconnection handling and regression coverage for 129025-only devices.
No full GUI/CommBridge integration build, hardware CAN test, watchdog failover
integration, or upstream PR has been completed.

Do not claim that this work is a fully resolved maritime navigation safety bug.
Tests are targeted, actual production method bodies; surrounding class/priority
infrastructure is simplified in the focused test.

## Open design questions

1. What is the canonical identity for the same N2K source across PGNs and
   multiple physical buses? Existing GetPriorityKey uses the N2K address.
2. Must an explicit no-fix report suspend 129025 from that source until a
   subsequent valid fix report? What about devices switching to 129025-only?
3. How should disconnect/reconnect, source-address reuse, and expiry reset
   source validity without unexpectedly reaccepting stale coordinates?
4. How can this be tested against existing file/SocketCAN driver interfaces
   with an actual CommBridge priority map?

## Next gate

Decide whether the upstream contribution should be narrowly scoped to
129029 method validation (small patch, accurate limited claim), or a
cross-PGN source validity change (more impact, substantially higher test
and contract risk). Do not merge either without appropriate integration tests.
