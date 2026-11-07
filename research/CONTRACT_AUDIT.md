# Scope & compatibility audit — GNSS PGN 129029 (2026-11-07)

## Decision

**Candidate for a narrowly scoped correctness PR**, **not** for a complete
GNSS failover PR. Limit production changes to the decoder method
`CommDecoder::DecodePGN129029()` in `model/src/comm_decoder.cpp`.
A six-line guard rejects explicitly invalid GNSS method values
`noGNSS = 0`, `Error = 14`, `Unavailable = 15` *before* copying coordinates
to `NavData`. Do not modify `PGN 129025`, the priority map, the watchdog,
UI, or autopilot in the same PR.

This **does not** guarantee that a device also sending PGN 129025 cannot
continue refreshing position. That question needs a separate source-state
design, physical bus identity analysis, and end-to-end integration tests.

## Verified source-level dependency and contract review

| Boundary | Existing contract | Result of isolated guard |
| --- | --- | --- |
| N2KParser PGN 129029 | Parse raw fields, including method; returns true on valid packet layout | **Unchanged**. Uses same parser result. |
| CommDecoder::DecodePGN129029 | Decode semantic navigation data. Existing RMC/GGA/GLL decoders also reject invalid fix | Returns false for methods 0,14,15 and leaves NavData coordinates unmodified. All other values retain current behavior. |
| CommBridge::HandleN2K_129029 | Returns early if decoder fails; otherwise EvalPriority, updates ownship position, POS_VALID and position watchdog; may report satellites | **For explicitly invalid methods** the handler exits before these side effects. |
| AppMsgBus / BasicNavDataMsg | Normal accepted updates published from bridge; UI updates bGPSValid on POS_UPDATE | On rejected packet there is no BasicNavDataMsg. UI's last validity state persists until the pre-existing watchdog expires. Do **not** claim immediate invalidation. |
| Watchdog and fallback | On timeout, clears velocity and expires active source priority, then considers next lower source | Stops the invalid 129029 packet from refreshing position watchdog; **does not** make switchover instantaneous. |
| N2K 129025 rapid position | Different listener and decoder; accepts numerically available lat/lon | **Unchanged**, intentionally, to preserve 129025-only devices. |
| GNSS method 1–5 | Valid GNSS, DGNSS, precise GNSS, RTK fixed and float | Unchanged and verified. |
| GNSS method 6–13 | Not defined by vendored enum | **Still accepted**; preserving behavior is compatibility, not evidence these statuses are valid fixes. |
| Source priority | GetPriorityKey includes device address and PGN; EvalPriority uses priority maps and active identifier | **Unchanged**; cannot assume same-address 129025 bypasses an active 129029 priority. |
| Satellite / velocity / heading | Other PGNs update their respective state; 129029 satellites are set only for method 1–3 | Guard does not modify those other code paths. Satellite reporting for invalid 129029 is suppressed; it was not populated by decoder for 0/14/15 before. |
| Raw N2K consumers / plugins | Message bus carries raw N2K PGNs independently of bridge's decode | Guard is inside CommDecoder and does not mutate or filter raw message delivery. |
| Autopilot and AIS | Both indirectly use bGPSValid and global ownship state | Their APIs unchanged; the *downstream data* may change when bad 129029 packets no longer refresh watchdog. This is intentional but requires integration regression. |

## Hidden contracts / why NOT to broaden this PR

1. **Not always a PGN 129025 bypass.** The earlier handler replay stubbed
   `EvalPriority()` to accept. In production `GetPriorityKey()` includes
   `PGN: 129029` or `PGN: 129025`, and the normal priority map can drop
   a lower-priority 129025 immediately. A bypass is conditional on priority
   configuration or watchdog-driven promotion, not proven universally.
2. **Source identity is not a stable arbitrary PGN join key.**
   `GetPriorityKey()` currently uses N2K source address and PGN. A new
   per-device state map could collide across buses, address reassignments,
   or driver restarts; adding a state map requires a separate design.
3. **Invalidity event semantics.** Existing bridge simply returns false for
   failed decoder cases. Emitting `POS_UPDATE` without `POS_VALID` changes UI
   and downstream autopilot semantics and could incorrectly invalidate an
   independently active alternative position source.
4. **Autopilot and downstream alerts.** `bGPSValid` governs route/autopilot
   output and influences AIS range/bearing. Rejecting bad 129029 will allow
   existing watchdog invalidation to occur, but a true integration test must
   demonstrate no valid-source regression.
5. **Partial vs complete GNSS validity.** The small patch does not manage
   129025-only devices, invalid GNSS method on a different PGN, implicit fix
   loss, or immediate failover. An unqualified "fix GNSS failover" PR title
   would be misleading.
6. **Parser vs semantic decoder.** N2KParser keeps returning parsed fields
   for no-fix packets, as it should. The semantic decoder rejects navigation
   updates; any future caller needing quality diagnostics should parse raw
   N2K metadata or introduce a separate quality API rather than silently
   assume decoded position exists.

## Reproducible tests

- Native N2KParser ASan/UBSan, methods 0/1–5/14/15:
  https://github.com/Johnny-Kao/opencpn-gnss-validity-research/actions/runs/42342246660
- Extracted unchanged production decoder: baseline fails expected invalid
  methods; candidate passes for **all 16 method values 0–15**, preserving 6–13;
  PGN 129025 independent behavior unchanged.
- Extracted unchanged production bridge handlers with **stubbed priority**:
  129025 can independently refresh watchdog after no-fix 129029 is refused,
  but this is NOT proof of production source selection.
- Last focused comparison:
  https://github.com/Johnny-Kao/opencpn-gnss-validity-research/actions/runs/42343902456
- Patch diff whitespace check: clean.

## Outstanding gates before upstream

1. Compile full OpenCPN `model-src` and its actual `tests` target on CI.
   The existing comparison currently compiles extracted **exact method
   bodies**, not the complete OpenCPN application.
2. Add a non-stubbed priority / watchdog integration test with controlled
   source ordering, including 129029-only, 129025-only, dual-PGN, primary
   failure, backup takeover and reconnection.
3. Verify emitted BasicNavDataMsg and GPSWatchdogMsg timing and autopilot
   validity flags under valid and invalid positions.
4. Check relevant active upstream issues, PRs and maintainer design
   expectations before considering a Draft PR.

**Current status:** a focused correction is evidence-backed and low in code
churn, but full production non-regression is **not yet proven**. The
cross-PGN source-state design has substantially higher risk and is excluded.
