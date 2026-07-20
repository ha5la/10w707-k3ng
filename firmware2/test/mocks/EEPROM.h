#pragma once
#include <stdint.h>
#include <string.h>

struct MockEEPROM {
  uint8_t data[128] = {0};
  template <typename T> T &get(int addr, T &v) {
    memcpy(&v, data + addr, sizeof(T));
    return v;
  }
  template <typename T> const T &put(int addr, const T &v) {
    memcpy(data + addr, &v, sizeof(T));
    return v;
  }
  void clear() { memset(data, 0, sizeof(data)); }
};
inline MockEEPROM EEPROM;
