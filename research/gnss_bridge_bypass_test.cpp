#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "research/gnss_bridge_shim.hpp"

static std::vector<unsigned char> frame(const tN2kMsg& msg) {
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
  CommBridge bridge;
  gLat = 0.0;
  gLon = 0.0;
  tN2kMsg nofix;
  SetN2kPGN129029(nofix, 2, 20000, 10.0, 35.6812, 139.7671,
                  5.0, N2kGNSSt_GPS, N2kGNSSm_noGNSS, 7, 0.8);
  auto m1 = std::make_shared<Nmea2000Msg>();
  m1->payload = frame(nofix);
  const bool accepted_129029 = bridge.HandleN2K_129029(m1);
  if (accepted_129029 || bridge.m_watchdogs.position_watchdog != 0 ||
      gLat != 0.0 || gLon != 0.0) {
    std::puts("FAIL: invalid 129029 modified navigation state");
    return EXIT_FAILURE;
  }
  tN2kMsg rapid;
  rapid.Source = nofix.Source; // same N2K address as no-fix message
  SetN2kPGN129025(rapid, 35.6812, 139.7671);
  auto m2 = std::make_shared<Nmea2000Msg>();
  m2->payload = frame(rapid);
  const bool accepted_129025 = bridge.HandleN2K_129025(m2);
  if (!accepted_129025 || bridge.m_watchdogs.position_watchdog != gps_watchdog_timeout_ticks ||
      std::abs(gLat - 35.6812) >= 1e-7) {
    std::puts("FAIL: expected independent 129025 position path");
    return EXIT_FAILURE;
  }
  std::puts("CONFIRMED: 129029 noGNSS rejected, but same-source 129025 refreshes watchdog");
  std::puts("LIMITATION: priority selection is stubbed; this is not full CommBridge integration");
  return EXIT_SUCCESS;
}
