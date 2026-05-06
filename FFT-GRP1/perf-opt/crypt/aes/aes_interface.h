//
// Created by fsc on 25-10-5.
//

#ifndef CRYPTION_AES_INTERFACE_H
#define CRYPTION_AES_INTERFACE_H

int aes_freertos_iopointer(int scale, void* input, void* output);
int aes_freertos_ioself_profiling(int scale);
int aes_freertos_ioself(int scale);

#endif //CRYPTION_AES_INTERFACE_H
