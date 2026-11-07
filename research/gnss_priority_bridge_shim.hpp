#pragma once
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "research/gnss_decoder_shim.hpp"

using string = std::string;
using PriorityMap = std::map<string, int>;
struct PriorityContainer {
  string prio_class;
  int active_priority;
  string active_source;
  string active_identifier;
  int active_source_address = -1;
  time_t recent_active_time = 0;
  explicit PriorityContainer(const string& cls, int prio = 0)
      : prio_class(cls), active_priority(prio) {}
};

class wxString {
 public:
  wxString() = default;
  wxString(const char* s) : value_(s ? s : "") {}
  wxString(string s) : value_(std::move(s)) {}
  string ToStdString() const { return value_; }
  const char* c_str() const { return value_.c_str(); }
  void Empty() { value_.clear(); }
  static wxString Format(const char* fmt, int x) {
    char b[160];
    std::snprintf(b, sizeof b, fmt, x);
    return wxString(b);
  }
  void Printf(const string& fmt, ...) {
    char b[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(b, sizeof b, fmt.c_str(), args);
    va_end(args);
    value_ = b;
  }
  void Printf(const char* fmt, ...) {
    char b[1024];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(b, sizeof b, fmt, args);
    va_end(args);
    value_ = b;
  }
  friend string operator+(const wxString& a, const char* b) {
    return a.value_ + b;
  }
 private:
  string value_;
};
class wxStringTokenizer {
 public:
  wxStringTokenizer(const wxString& s, const char* delimiter) {
    auto v = s.ToStdString();
    auto next = v.find_first_not_of(delimiter, 0);
    while (next != string::npos) {
      auto end = v.find_first_of(delimiter, next);
      tokens_.push_back(v.substr(next, end-next));
      next = v.find_first_not_of(delimiter, end);
    }
  }
  wxString GetNextToken() {
    if (index_ >= tokens_.size()) return wxString();
    return wxString(tokens_[index_++]);
  }
  bool HasMoreTokens() const { return index_ < tokens_.size(); }
 private:
  std::vector<string> tokens_;
  size_t index_ = 0;
};
struct wxDateTime {
  static wxDateTime Now() { return {}; }
  time_t GetTicks() const { return 12345; }
};
inline int wxMin(int a, int b) { return std::min(a, b); }
inline const char* _(const char* s) { return s; }
inline void wxLogMessage(const wxString&) {}
inline void wxLogMessage(const char*) {}

struct NavAddr {
  enum class Bus { N0183, N2000, Signalk };
  virtual ~NavAddr() = default;
  virtual string to_string() const { return ""; }
};
struct NavAddrSignalK : NavAddr {};
struct NavMsg {
  NavAddr::Bus bus = NavAddr::Bus::N2000;
  std::shared_ptr<const NavAddr> source;
  virtual ~NavMsg() = default;
};
struct Pgn { int number; string to_string() const { return std::to_string(number); } };
struct Nmea2000Msg : NavMsg {
  std::vector<unsigned char> payload;
  Pgn PGN{0};
  Nmea2000Msg() { bus = NavAddr::Bus::N2000; }
};
struct Nmea0183Msg : NavMsg {
  string talker, type;
};
struct SignalkMsg : NavMsg {};
using NavMsgPtr = std::shared_ptr<const NavMsg>;
using N2000MsgPtr = std::shared_ptr<const Nmea2000Msg>;

enum class NotificationSeverity { kInformational };
struct NotificationManager {
  static NotificationManager& GetInstance() { static NotificationManager n; return n; }
  void AddNotification(NotificationSeverity, const string&) {}
};

inline constexpr int POS_UPDATE = 1;
inline constexpr int POS_VALID = 2;
inline constexpr int N_ACTIVE_LOG_WATCHDOG = 5;
inline constexpr int gps_watchdog_timeout_ticks = 5;
inline constexpr int sat_watchdog_timeout_ticks = 5;

inline double gLat = 0, gLon = 0, gSog = 0, gCog = 0, gHdt = 0;
inline double gVar = 0;
inline wxString gRmcDate, gRmcTime;
inline bool g_bSatValid = false, g_bVAR_Rx = false;
inline int g_SatsInView = 0, g_priSats = 99, g_nNMEADebug = 0;
inline bool debug_priority = false;
inline int basic_nav_count = 0;
inline int basic_last_flags = 0;
inline int wd_position_events = 0;
inline int wd_velocity_events = 0;
inline void SendBasicNavdata(int flag, bool) {
  ++basic_nav_count;
  basic_last_flags = flag;
}
inline void ClearNavData(NavData& d) { d = NavData{}; }

struct GPSWatchdogMsg {
  enum class WDSource { position, velocity, heading };
  WDSource wd_source;
  int gps_watchdog;
  GPSWatchdogMsg(WDSource source, int ticks)
      : wd_source(source), gps_watchdog(ticks) {}
};
struct AppMsgBus {
  static AppMsgBus& GetInstance() { static AppMsgBus b; return b; }
  void Notify(std::shared_ptr<GPSWatchdogMsg> msg) {
    if (msg->wd_source == GPSWatchdogMsg::WDSource::position) ++wd_position_events;
    if (msg->wd_source == GPSWatchdogMsg::WDSource::velocity) ++wd_velocity_events;
  }
};
inline void LogAppMsg(const std::shared_ptr<GPSWatchdogMsg>&, const char*, bool) {}

class CommBridge {
 public:
  struct Watchdogs {
    int position_watchdog = 0;
    int velocity_watchdog = 0;
    int heading_watchdog = 0;
    int variation_watchdog = 0;
    int satellite_watchdog = 0;
  };
  CommDecoder m_decoder;
  Watchdogs m_watchdogs;
  PriorityContainer active_priority_position{"position"};
  PriorityContainer active_priority_velocity{"velocity"};
  PriorityContainer active_priority_heading{"heading"};
  PriorityContainer active_priority_variation{"variation"};
  PriorityContainer active_priority_satellites{"satellites"};
  PriorityMap priority_map_position, priority_map_velocity, priority_map_heading;
  PriorityMap priority_map_variation, priority_map_satellites;
  int m_n_log_watchdog_period = 5;
  int m_last_position_priority = 0;
  string m_last_position_source;
  bool m_log_callbacks = false;
  bool EvalPriority(const NavMsgPtr& msg, PriorityContainer&, PriorityMap&);
  void OnWatchdogTimer();
  bool HandleN2K_129029(const N2000MsgPtr& msg);
  bool HandleN2K_129025(const N2000MsgPtr& msg);
};
