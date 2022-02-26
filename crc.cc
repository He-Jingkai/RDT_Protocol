#include "crc.h"

#define WIDTH (8 * sizeof(u_int8_t))
#define TOPBIT (1 << (WIDTH - 1))
#define POLYNOMIAL 0xD8

namespace crc {

u_int8_t crc4_itu(char const message[], int nBytes) {
  u_int8_t i;
  u_int8_t crc = 0xFF;  // Initial value
  while (nBytes--) {
    crc ^= *message++;  // crc ^= *data; data++;
    for (i = 0; i < 8; ++i) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xE0;  // 0xE0 = reverse 0x07
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}

u_int8_t crc8_rohc(char const message[], int nBytes) {
  u_int8_t i;
  u_int8_t crc = 0;  // Initial value
  while (nBytes--) {
    crc ^= *message++;  // crc ^= *data; data++;
    for (i = 0; i < 8; ++i) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0x0C;  // 0x0C = (reverse 0x03)>>(8-4)
      else
        crc = (crc >> 1);
    }
  }
  return crc;
}
}  // namespace crc
