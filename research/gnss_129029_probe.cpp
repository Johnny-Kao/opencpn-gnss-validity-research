// Standalone research probe using OpenCPN's vendored N2KParser implementation.
// This does not compile or invoke CommBridge/CommDecoder. Do not mistake this
// probe for an integration regression test.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "N2kMessages.h"

// N2KParser normally obtains this from its embedding application. The probe
// does not use elapsed time; a fixed tick is sufficient for message creation.
uint32_t millis() { return 0; }

static void Check(bool ok, const char* what) {
  if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}

static bool HasUsableGnssFix(tN2kGNSSmethod m) {
  return m == N2kGNSSm_GNSSfix || m == N2kGNSSm_DGNSS ||
         m == N2kGNSSm_PreciseGNSS || m == N2kGNSSm_RTKFixed ||
         m == N2kGNSSm_RTKFloat;
}

int main() {
  constexpr double kLat = 35.6812;
  constexpr double kLon = 139.7671;
  for (int raw : {0, 1, 2, 3, 4, 5, 14, 15}) {
    const auto method = static_cast<tN2kGNSSmethod>(raw);
    tN2kMsg msg;
    SetN2kPGN129029(msg, 2, 20000, 12.0, kLat, kLon, 10.0,
                    N2kGNSSt_GPS, method, 7, 0.8);
    unsigned char sid = 0, satellites = 0, refs = 0;
    uint16_t date = 0, refid = 0;
    double time = 0, lat = 0, lon = 0, alt = 0, hdop = 0, pdop = 0;
    double geoid = 0, age = 0;
    tN2kGNSStype gnss_type = N2kGNSSt_GPS;
    tN2kGNSSmethod parsed_method = N2kGNSSm_Unavailable;
    tN2kGNSStype ref_type = N2kGNSSt_GPS;
    bool parsed = ParseN2kPGN129029(msg, sid, date, time, lat, lon, alt,
                    gnss_type, parsed_method, satellites, hdop, pdop, geoid,
                    refs, ref_type, refid, age);
    Check(parsed, "native PGN parser rejected test packet");
    Check(parsed_method == method, "native parser lost GNSS method");
    Check(std::abs(lat - kLat) < 1e-7 && std::abs(lon - kLon) < 1e-7,
          "native parser changed coordinates");
    bool expected = raw >= 1 && raw <= 5;
    Check(HasUsableGnssFix(parsed_method) == expected,
          "GNSS fix classification is unexpected");
    // Current CommDecoder::DecodePGN129029 copies coordinates even when
    // method reports noGNSS/Error/Unavailable; this demonstrates that native
    // packets can carry valid-looking coordinates alongside invalid fix.
    std::printf("method=%2d parsed=yes coordinates=present fix_usable=%s\n",
                raw, expected ? "yes" : "no");
  }
  std::puts("PASS: native N2KParser GNSS method boundary probe");
}
