#pragma once
#include <memory>
#include <vector>
#include "research/gnss_decoder_shim.hpp"

struct Nmea2000Msg { std::vector<unsigned char> payload; };
using N2000MsgPtr = std::shared_ptr<const Nmea2000Msg>;

inline void ClearNavData(NavData& d) { d = NavData{}; }
inline constexpr int POS_UPDATE = 1;
inline constexpr int POS_VALID = 2;
inline constexpr int N_ACTIVE_LOG_WATCHDOG = 5;
inline constexpr int gps_watchdog_timeout_ticks = 10;
inline constexpr int sat_watchdog_timeout_ticks = 10;
inline double gLat = 0;
inline double gLon = 0;
inline int g_SatsInView = 0;
inline bool g_bSatValid = false;
inline int last_vflag = 0;
inline void SendBasicNavdata(int vflag, bool) { last_vflag = vflag; }

class CommBridge {
 public:
  struct Watchdogs { int position_watchdog = 0; int satellite_watchdog = 0; };
  CommDecoder m_decoder;
  Watchdogs m_watchdogs;
  int m_n_log_watchdog_period = 0;
  int active_priority_position = 0;
  int priority_map_position = 0;
  int active_priority_satellites = 0;
  int priority_map_satellites = 0;
  bool m_log_callbacks = false;
  bool EvalPriority(const N2000MsgPtr&, int&, int&) { return true; } // isolate validity only
  bool HandleN2K_129029(const N2000MsgPtr& msg);
  bool HandleN2K_129025(const N2000MsgPtr& msg);
};
