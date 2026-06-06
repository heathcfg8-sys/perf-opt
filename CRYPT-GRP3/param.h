//
// Created by 34701 on 25-7-1.
//

/*
 * Build-time feature flags controlling micro-optimizations for crypt routines.
 * Keep this header included before algorithm sources so macros take effect.
 */

#ifndef CRYPTION_PARAM_H
#define CRYPTION_PARAM_H

// MD5 tuning: word access, message buffer reuse, and loop unrolling.
#define MD5MemCopyOpt
#define MD5MallocOpt
#define MD5LoopUnroll
// SHA256 tuning: remove extra masking and add branch prediction hints.
#define SHA256RremoveUselessCal
#define SHA256BranchPred
// AES tuning: use merged ShiftRows implementation.
#define AESMergeShift

#endif //CRYPTION_PARAM_H
