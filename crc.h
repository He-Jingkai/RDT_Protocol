#include <stdint.h>

namespace crc {
uint8_t crc4_itu(char const message[], int nBytes);
uint8_t crc8_rohc(char const message[], int nBytes);
uint8_t crc6_itu(char const message[], int nBytes);
}  // namespace crc