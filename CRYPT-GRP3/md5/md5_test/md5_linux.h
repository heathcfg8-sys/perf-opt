#ifndef MD5_LINUX_H
#define MD5_LINUX_H

#include <stddef.h>
#include <stdint.h>

typedef enum RetCode_e {
    K_RET_UNK_ERROR = -1,
    K_RET_OK = 0,
    K_RET_TIMEOUT,
    K_RET_INVALID_PARAM,
    K_RET_BUSY,
} RetCode_t;

RetCode_t md5_linux_iopointer(int horizon, void *input, void *output);
RetCode_t md5_linux_ioself_profiling(int horizon);
RetCode_t md5_linux_ioself(int horizon);

#endif
