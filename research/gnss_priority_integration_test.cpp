#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include "research/gnss_priority_bridge_shim.hpp"

static int failures = 0;
static void expect(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok) ++failures;
}
static std::vector<unsigned char> frame(const tN2kMsg& m) {
  std::vector<unsigned char> v{0x93, static_cast<unsigned char>(m.DataLen + 11), m.Priority};
  for (int i = 0; i < 3; ++i) v.push_back((m.PGN >> (i * 8)) & 255);
  v.push_back(m.Destination);
  v.push_back(m.Source);
  for (int i = 0; i < 4; ++i) v.push_back((m.MsgTime >> (i * 8)) & 255);
  v.push_back(static_cast<unsigned char>(m.DataLen));
  for (int i = 0; i < m.DataLen; ++i) v.push_back(m.Data[i]);
  v.push_back(0);
  return v;
}
static N2000MsgPtr position(int source, int method, double lat) {
  tN2kMsg m;
  SetN2kPGN129029(m, 2, 20000, 10.0, lat, 139.7671, 5.0,
                  N2kGNSSt_GPS, static_cast<tN2kGNSSmethod>(method), 7, 0.8);
  m.Source = source;
  auto p = std::make_shared<Nmea2000Msg>();
  p->payload = frame(m);
  p->PGN.number = 129029;
  return p;
}
static N2000MsgPtr rapid(int source, double lat) {
  tN2kMsg m;
  SetN2kPGN129025(m, lat, 139.7671);
  m.Source = source;
  auto p = std::make_shared<Nmea2000Msg>();
  p->payload = frame(m);
  p->PGN.number = 129025;
  return p;
}
static std::string key(int src, int pgn) {
  return "N2k device address: " + std::to_string(src) + " ; PGN: " +
         std::to_string(pgn);
}
static void reset() {
  gLat = 0;
  gLon = 0;
  basic_nav_count = 0;
  basic_last_flags = 0;
  wd_position_events = 0;
}
static void tick(CommBridge& b, int times = gps_watchdog_timeout_ticks) {
  for (int i = 0; i < times; ++i) b.OnWatchdogTimer();
}

int main() {
  // Actual OpenCPN handlers, source priority and watchdog bodies are compiled.
  // Only UI/notification and app-message-bus dependencies are stand-ins.
  {
    reset();
    CommBridge b;
    b.priority_map_position[key(11, 129029)] = 0;
    b.priority_map_position[key(22, 129029)] = 1;
    expect(b.HandleN2K_129029(position(11, 1, 35.0)), "primary fix accepted");
    expect(b.m_watchdogs.position_watchdog == 5 &&
           std::abs(gLat-35.0)<1e-7, "primary position/watchdog set");
    b.HandleN2K_129029(position(22, 1, 40.0));
    expect(std::abs(gLat-35.0)<1e-7,
           "actual EvalPriority rejects backup while primary active");
    const int event_before = basic_nav_count;
    expect(!b.HandleN2K_129029(position(11, 0, 35.0)),
           "no-fix rejected in real handler");
    expect(basic_nav_count == event_before &&
           b.m_watchdogs.position_watchdog == 5,
           "no-fix leaves message bus and watchdog unchanged");
    tick(b);
    expect(b.active_priority_position.active_priority == 1,
           "real OnWatchdogTimer moves priority to backup");
    expect(wd_position_events > 0, "watchdog message emitted");
    b.HandleN2K_129029(position(22, 1, 40.0));
    expect(std::abs(gLat-40.0)<1e-7 &&
           b.m_watchdogs.position_watchdog == 5,
           "backup accepted after timeout and refreshes watchdog");
    b.HandleN2K_129029(position(11, 1, 35.0));
    expect(std::abs(gLat-35.0)<1e-7 &&
           b.active_priority_position.active_priority == 0,
           "restored primary reclaims higher priority");
  }
  {
    reset();
    CommBridge b;
    b.priority_map_position[key(33, 129025)] = 0;
    b.HandleN2K_129025(rapid(33, 42.0));
    expect(std::abs(gLat-42.0)<1e-7 &&
           b.m_watchdogs.position_watchdog == 5,
           "129025-only source retains accepted position behavior");
  }
  {
    reset();
    CommBridge b;
    b.priority_map_position[key(11, 129029)] = 0;
    b.priority_map_position[key(11, 129025)] = 1;
    b.priority_map_position[key(22, 129029)] = 2;
    b.HandleN2K_129029(position(11, 1, 35.0));
    b.HandleN2K_129025(rapid(11, 36.0));
    expect(std::abs(gLat-35.0)<1e-7,
           "real priority initially blocks lower-priority 129025");
    expect(!b.HandleN2K_129029(position(11, 14, 35.0)),
           "error status refused on primary");
    tick(b);
    expect(b.active_priority_position.active_priority == 1,
           "watchdog picks lower-priority same-device 129025 before backup");
    b.HandleN2K_129025(rapid(11, 36.0));
    expect(std::abs(gLat-36.0)<1e-7 &&
           b.m_watchdogs.position_watchdog == 5,
           "CONFIRMED: real priority can accept same-device 129025 after no-fix");
    b.HandleN2K_129029(position(22, 1, 40.0));
    expect(std::abs(gLat-36.0)<1e-7,
           "configured lower-priority backup still blocked by 129025");
  }
  std::printf("nonstub-method integration: %s (%d failures)\n",
              failures ? "FAIL" : "PASS", failures);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
