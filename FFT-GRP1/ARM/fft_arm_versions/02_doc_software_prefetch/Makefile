TARGET = fft
CALL_TARGET = call_test
SOURCES = main.c fft_a.c timestamp.c
CALL_SOURCES = call_test.c fft_a.c timestamp.c
CC ?= gcc
CROSS_CC ?= aarch64-linux-gnu-gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
ARM_CFLAGS ?= $(CFLAGS) -march=armv8-a+simd -static
LDLIBS = -lm

.PHONY: native arm call_native call_arm clean

native:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDLIBS)

arm:
	$(CROSS_CC) $(ARM_CFLAGS) -o $(TARGET) $(SOURCES) $(LDLIBS)

call_native:
	$(CC) $(CFLAGS) -o $(CALL_TARGET) $(CALL_SOURCES) $(LDLIBS)

call_arm:
	$(CROSS_CC) $(ARM_CFLAGS) -o $(CALL_TARGET) $(CALL_SOURCES) $(LDLIBS)

clean:
	rm -f $(TARGET) $(CALL_TARGET) *.o qemu_result.txt native_result.txt
