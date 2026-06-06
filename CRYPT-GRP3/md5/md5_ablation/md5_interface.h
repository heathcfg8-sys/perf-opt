//
// Created by fsc on 25-10-5.
//

/*
 * md5_interface.h - MD5 test/benchmark entry points.
 * These functions are used by the sample main program and profiling loops.
 */

#ifndef CRYPTION_AES_INTERFACE_H
#define CRYPTION_INTERFACE_H

// Compute MD5 on caller-provided buffers.
int md5_freertos_iopointer(int scale, void* input, void* output);
// Allocate buffers, run MD5, and print timing + digest.
int md5_freertos_ioself_profiling(int scale);
// Allocate buffers and run MD5 (no timing output).
int md5_freertos_ioself(int scale);

#endif //CRYPTION_AES_INTERFACE_H
