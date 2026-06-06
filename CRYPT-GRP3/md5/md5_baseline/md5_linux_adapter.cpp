#include "md5.h"

extern "C" void md5_linux_compute(const uint8_t *input, size_t len, uint8_t *digest)
{
    md5(input, len, digest);
}
