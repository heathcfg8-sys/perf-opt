#ifndef CRYPTION_PARAM_H
#define CRYPTION_PARAM_H

// ==== MD5 优化开关 ====
#define MD5MemCopyOpt
#define MD5MallocOpt
#define MD5LoopUnroll

// ==== SHA256 优化开关 (默认全部注释，即全关闭状态) ====
// 优化一：去除无用计算与分支预测 (Teacher)
#define SHA256RremoveUselessCal
#define SHA256BranchPred

// 优化二：Padding 填充优化 (Memset)
#define SHA256PaddingOpt

// 优化三：数据预取优化 (Prefetch)
#define SHA256PrefetchOpt

// 优化四：循环展开优化 (Unroll)
#define SHA256UnrollOpt

// ==== AES 优化开关 ====
#define AESMergeShift

#endif //CRYPTION_PARAM_H