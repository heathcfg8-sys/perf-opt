#!/usr/bin/env bash
set -euo pipefail

: "${MD5_PAD_OPT:=0}"
: "${MD5_ALIGN_OPT:=0}"
: "${MD5_PREFETCH_OPT:=0}"
: "${MD5_UNROLL_OPT:=0}"

COMMON_CFLAGS="-O3 -I./ -I.. -I./md5_ablation -I./md5_test"
MD5_DEFS="-DMD5_PAD_OPT=${MD5_PAD_OPT} -DMD5_ALIGN_OPT=${MD5_ALIGN_OPT} -DMD5_PREFETCH_OPT=${MD5_PREFETCH_OPT} -DMD5_UNROLL_OPT=${MD5_UNROLL_OPT}"

echo "MD5 flags: ${MD5_DEFS}"

gcc ${COMMON_CFLAGS} -c md5_test/md5_linux.c -o md5_linux.o
gcc ${COMMON_CFLAGS} -c md5_test/md5_linux_test.c -o md5_linux_test.o
gcc ${COMMON_CFLAGS} -c md5_test/timestamp.c -o timestamp.o
g++ ${COMMON_CFLAGS} ${MD5_DEFS} -c md5_ablation/md5.cpp -o md5.o
g++ ${COMMON_CFLAGS} ${MD5_DEFS} -c md5_ablation/md5_linux_adapter.cpp -o md5_linux_adapter.o
g++ md5_linux.o md5_linux_test.o timestamp.o md5.o md5_linux_adapter.o -o md5_test_aba
./md5_test_aba