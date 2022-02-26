#include <stdlib.h>

namespace crc {
u_int8_t crc4_itu(char const message[], int nBytes);
u_int8_t crc8_rohc(char const message[], int nBytes);
}  // namespace crc