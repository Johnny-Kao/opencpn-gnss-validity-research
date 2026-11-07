# OpenCPN GNSS validity research

This public repository is an independent **copy** of OpenCPN `master`, not a
GitHub fork. Upstream: https://github.com/OpenCPN/OpenCPN

## Purpose

Investigate whether NMEA 2000 PGN 129029 packets reporting `noGNSS`,
`Error` or `Unavailable` can refresh ownship position and watchdog state
when their coordinate fields remain numerically valid.

The current code path of interest is:

- `libs/N2KParser/src/N2kMessages.cpp` parses the GNSS method field.
- `model/src/comm_decoder.cpp` (`DecodePGN129029`) assigns coordinates
  without rejecting invalid GNSS methods.
- `model/src/comm_bridge.cpp` (`HandleN2K_129029`) accepts non-NA
  coordinates, raises `POS_VALID` and resets the position watchdog.
- `HandleN2K_129025` is a separate rapid-position path without an
  independent GNSS fix method; avoid regressing 129025-only devices.

## Validation levels

1. **Native parser probe (implemented):** compile real vendored N2KParser
   sources and verify that no-fix PGN 129029 messages can contain valid-looking
   lat/lon, and GNSS method survives decoding. ASan/UBSan enabled.
2. **CommDecoder test (pending):** invoke production decoder with the same
   packets and capture its return / state.
3. **CommBridge integration (pending):** exercise POS_VALID, watchdog, source
   priority and fallback. Check ordering, timeouts and method transitions.
4. **Contract regression (pending):** ensure RTK Fixed/Float, common valid
   fixes and PGN 129025-only device flows stay supported.

No upstream PR or changes to the user's other repositories are made here.
No operational navigation safety guarantee is inferred from a parser probe.

## CI

Upstream CI workflows were moved to `research/upstream-workflows/` to avoid
accidentally launching an expensive multi-platform matrix on the first push.
Use the manually dispatched **GNSS 129029 focused research** workflow.

Upstream licensing and attribution remain unchanged.
