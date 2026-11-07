#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include "N2KParser.h"
#include "N2kMessages.h"

struct NavData {
  double gLat = NAN;
  double gLon = NAN;
  int n_satellites = -1;
  int SID = -1;
};

class CommDecoder {
 public:
  bool DecodePGN129029(std::vector<unsigned char> v, NavData& data);
  bool DecodePGN129025(std::vector<unsigned char> v, NavData& data);
};
