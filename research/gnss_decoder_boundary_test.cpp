// Production function extraction tests, not full OpenCPN integration.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <vector>
#include "research/gnss_decoder_shim.hpp"

static int failures = 0;
static void check(bool ok, const char* test) {
  if (!ok) { ++failures; std::printf("FAIL: %s\n", test); }
}
static std::vector<unsigned char> actisense(const tN2kMsg& msg) {
  std::vector<unsigned char> v{0x93, static_cast<unsigned char>(msg.DataLen+11),
                                msg.Priority};
  for (int i=0; i<3; ++i) v.push_back((msg.PGN >> (i*8)) & 0xff);
  v.push_back(msg.Destination);
  v.push_back(msg.Source);
  for (int i=0; i<4; ++i) v.push_back((msg.MsgTime >> (i*8)) & 0xff);
  v.push_back(static_cast<unsigned char>(msg.DataLen));
  for (int i=0; i<msg.DataLen; ++i) v.push_back(msg.Data[i]);
  v.push_back(0);
  return v;
}
int main() {
  CommDecoder decoder;
  for (int method : {0,1,2,3,4,5,14,15}) {
    tN2kMsg p;
    SetN2kPGN129029(p, 2, 20000, 10.0, 35.6812, 139.7671, 5.0,
                    N2kGNSSt_GPS, static_cast<tN2kGNSSmethod>(method),
                    7, 0.8);
    NavData data;
    const bool result = decoder.DecodePGN129029(actisense(p), data);
    const bool expected = method != 0 && method != 14 && method != 15;
    std::printf("GNSS method=%d decoded=%d expected=%d\n", method, result, expected);
    check(result == expected, "129029 GNSS validity status");
    check(std::isnan(data.gLat) == !expected, "129029 invalid coordinate isolation");
    if (expected) {
      check(std::abs(data.gLat-35.6812)<1e-7, "129029 valid latitude");
      check(std::abs(data.gLon-139.7671)<1e-7, "129029 valid longitude");
    }
  }
  tN2kMsg rapid;
  SetN2kPGN129025(rapid, 35.6812, 139.7671);
  NavData d;
  check(decoder.DecodePGN129025(actisense(rapid), d), "129025-only device path");
  check(std::abs(d.gLat-35.6812)<1e-7 &&
        std::abs(d.gLon-139.7671)<1e-7, "129025 coordinates");
  std::printf("%s, failures=%d\n", failures ? "FAIL" : "PASS", failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
