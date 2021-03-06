#pragma once

#include "pebble.h"

#define NUM_CLOCK_TICKS 13

static const struct GPathInfo ANALOG_BG_POINTS[] = {
  { 4,
    (GPoint []) {
      {68, 0},
      {71, 0},
      {71, 12},
      {68, 12}
    }
  },
  { 4, (GPoint []){
      {72, 0},
      {75, 0},
      {75, 12},
      {72, 12}
    }
  },
  { 4, (GPoint []){
      {112, 10},
      {114, 12},
      {108, 23},
      {106, 21}
    }
  },
  { 4, (GPoint []){
      {132, 47},
      {144, 40},
      {144, 44},
      {135, 49}
    }
  },
  { 4, (GPoint []){ //3
      {140, 83},
      {154, 83},
      {154, 85},
      {140, 85}
    }
  },
  { 4, (GPoint []){
      {135, 118},
      {144, 123},
      {144, 126},
      {132, 120}
    }
  },
  { 4, (GPoint []){
      {108, 144},
      {114, 154},
      {112, 157},
      {106, 147}
    }
  },
  { 4, (GPoint []){ //6
      {70, 155},
      {73, 155},
      {73, 167},
      {70, 167}
    }
  },
  { 4, (GPoint []){
      {32, 10},
      {30, 12},
      {36, 23},
      {38, 21}
    }
  },
  { 4, (GPoint []){
      {12, 47},
      {-1, 40},
      {-1, 44},
      {9, 49}
    }
  },
  { 4, (GPoint []){ // 9
      {-10, 83},
      {4, 83},
      {4, 85},
      {-10, 85}
    }
  },
  { 4, (GPoint []){
      {9, 118},
      {-1, 123},
      {-1, 126},
      {12, 120}
    }
  },
  { 4, (GPoint []){
      {36, 144},
      {30, 154},
      {32, 157},
      {38, 147}
    }
  },

};

// 長針の図形
static const GPathInfo MINUTE_HAND_POINTS = {
  3, (GPoint []) {
    { -6, 10 },
    { 6, 10 },
    { 0, -80 }
  }
};

// 短針の図形
static const GPathInfo HOUR_HAND_POINTS = {
  3, (GPoint []){
    {-8, 10},
    {8, 10},
    {0, -50}
  }
};

// バイブレーションパターンの定義
#define ON_S 200
#define OFF_S 200
#define ON_L 400
#define OFF_L 200

static const uint32_t rom_BT[] = { 100,100, 400,100, 300,100, 100 };
static const uint32_t rom_01[] = { ON_S };
static const uint32_t rom_02[] = { ON_S, OFF_S, ON_S };
static const uint32_t rom_03[] = { ON_S, OFF_S, ON_S, OFF_S, ON_S };
static const uint32_t rom_04[] = { ON_S, OFF_S, ON_L };
static const uint32_t rom_05[] = { ON_L };
static const uint32_t rom_06[] = { ON_L, OFF_L, ON_S };
static const uint32_t rom_07[] = { ON_L, OFF_L, ON_S, OFF_S, ON_S };
static const uint32_t rom_08[] = { ON_L, OFF_L, ON_S, OFF_S, ON_S, OFF_S, ON_S };
static const uint32_t rom_09[] = { ON_S, OFF_S, ON_L, OFF_L, ON_L };
static const uint32_t rom_10[] = { ON_L, OFF_L, ON_L };
static const uint32_t rom_11[] = { ON_L, OFF_L, ON_L, OFF_L, ON_S };
static const uint32_t rom_12[] = { ON_L, OFF_L, ON_L, OFF_L, ON_S, OFF_S, ON_S };

VibePattern pat_BT = {
  .durations = rom_BT,
  .num_segments = ARRAY_LENGTH(rom_BT),
};
VibePattern pat_01 = {
  .durations = rom_01,
  .num_segments = ARRAY_LENGTH(rom_01),
};
VibePattern pat_02 = {
  .durations = rom_02,
  .num_segments = ARRAY_LENGTH(rom_02),
};
VibePattern pat_03 = {
  .durations = rom_03,
  .num_segments = ARRAY_LENGTH(rom_03),
};
VibePattern pat_04 = {
  .durations = rom_04,
  .num_segments = ARRAY_LENGTH(rom_04),
};
VibePattern pat_05 = {
  .durations = rom_05,
  .num_segments = ARRAY_LENGTH(rom_05),
};
VibePattern pat_06 = {
  .durations = rom_06,
  .num_segments = ARRAY_LENGTH(rom_06),
};
VibePattern pat_07 = {
  .durations = rom_07,
  .num_segments = ARRAY_LENGTH(rom_07),
};
VibePattern pat_08 = {
  .durations = rom_08,
  .num_segments = ARRAY_LENGTH(rom_08),
};
VibePattern pat_09 = {
  .durations = rom_09,
  .num_segments = ARRAY_LENGTH(rom_09),
};
VibePattern pat_10 = {
  .durations = rom_10,
  .num_segments = ARRAY_LENGTH(rom_10),
};
VibePattern pat_11 = {
  .durations = rom_11,
  .num_segments = ARRAY_LENGTH(rom_11),
};
VibePattern pat_12 = {
  .durations = rom_12,
  .num_segments = ARRAY_LENGTH(rom_12),
};

