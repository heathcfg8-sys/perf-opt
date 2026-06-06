# 文件说明

参见代码文件中的`readme`

# MD5 构建与消融实验说明

该目录包含三种 MD5 变体以及一个小型基准测试框架。

## 目录结构

* **md5_baseline/**：参考实现
* **md5_ablation/**：用于消融实验的实现（基于特性开关）
* **md5_optimized/**：优化版本（启用所有优化）
* **md5_test/**：基准测试辅助工具（Linux 测试与时间戳）
* **md5_baseline.sh / md5_aba.sh / md5_optimized.sh**：构建与运行脚本

## md5_ablation 特性开关

消融构建通过编译期开关控制各项优化。这些开关在 `md5_ablation/md5.cpp` 中读取，并可通过 `md5_ablation.sh` 中的环境变量进行设置。

* **MD5_PAD_OPT**：O(1) padding 长度计算
* **MD5_ALIGN_OPT**：对齐分配 + 对齐字访问
* **MD5_PREFETCH_OPT**：预取下一个 64 字节数据块
* **MD5_UNROLL_OPT**：在 AArch64 上完全展开轮函数

所有开关取值为 0 或 1。例如：

```bash
MD5_PAD_OPT=1 MD5_ALIGN_OPT=0 MD5_PREFETCH_OPT=0 MD5_UNROLL_OPT=0 ./md5_aba.sh
```

## AArch64 / NEON 说明

* **MD5_UNROLL_OPT** 仅在 AArch64 架构上生效
* 要实现真正的 SIMD/NEON 加速 MD5，需要批量处理多个独立消息（例如 4 路或 8 路并行哈希）
* 单消息 MD5 在每个 512-bit 块之间存在严格数据依赖，因此除非重构为多缓冲处理，否则 NEON 提升有限

## 基准测试说明

测试程序会对不同输入规模进行测量：

* 128 字节
* 256 字节
* 512 字节
* …
* 最大到 1 MiB

# 最终优化结果

![Code_Generated_Image](https://cdn.jsdelivr.net/gh/dongliangnie/image@master/image/202606031025950.png)

~~~
[dongliang@openEuler crypt]$ ./md5_optimized.sh
[scale=128 Bytes] MD5 time: 14205 ns
Digest: 37eff01866ba3f538421b30b7cbefcac
[scale=256 Bytes] MD5 time: 12390 ns
Digest: e2c865db4162bed963bfaa9ef6ac18f0
[scale=512 Bytes] MD5 time: 2816 ns
Digest: f5c8e3c31c044bae0e65569560b54332
[scale=1024 Bytes] MD5 time: 5090 ns
Digest: b2ea9f7fcea831a4a63b213f41a8855b
[scale=2048 Bytes] MD5 time: 12306 ns
Digest: 1576a94d6cb334dd126cb1c27f19e0f2
[scale=4096 Bytes] MD5 time: 20545 ns
Digest: 2bcd3c4de20c918e19fab5c36249c70d
[scale=8192 Bytes] MD5 time: 34417 ns
Digest: 6556112372898c69e1de0bf689d8db26
[scale=16384 Bytes] MD5 time: 77573 ns
Digest: 3df67097cee5e4cea36e0f941c134ffc
[scale=32768 Bytes] MD5 time: 151912 ns
Digest: 315a5931d0f93fd1f62a15d77cb234ef
[scale=65536 Bytes] MD5 time: 350361 ns
Digest: 8f1445bafe2c2095044af7789462f475
[scale=131072 Bytes] MD5 time: 661695 ns
Digest: 38a503307786991a982f8ded498b90e0
[scale=262144 Bytes] MD5 time: 1201409 ns
Digest: d19215b1d714757e1fdb0060c52fd4c8
[scale=524288 Bytes] MD5 time: 2430351 ns
Digest: b86919af2c4e30c5e717b40641aa0ef2
[scale=1048576 Bytes] MD5 time: 4855467 ns
Digest: c35cc7d8d91728a0cb052831bc4ef372
[dongliang@openEuler crypt]$
~~~

# baseline结果

~~~
[dongliang@openEuler crypt]$ ./md5_baseline.sh
[scale=128 Bytes] MD5 time: 6937 ns
Digest: 37eff01866ba3f538421b30b7cbefcac
[scale=256 Bytes] MD5 time: 2333 ns
Digest: e2c865db4162bed963bfaa9ef6ac18f0
[scale=512 Bytes] MD5 time: 3417 ns
Digest: f5c8e3c31c044bae0e65569560b54332
[scale=1024 Bytes] MD5 time: 8958 ns
Digest: b2ea9f7fcea831a4a63b213f41a8855b
[scale=2048 Bytes] MD5 time: 14833 ns
Digest: 1576a94d6cb334dd126cb1c27f19e0f2
[scale=4096 Bytes] MD5 time: 22667 ns
Digest: 2bcd3c4de20c918e19fab5c36249c70d
[scale=8192 Bytes] MD5 time: 81773 ns
Digest: 6556112372898c69e1de0bf689d8db26
[scale=16384 Bytes] MD5 time: 94752 ns
Digest: 3df67097cee5e4cea36e0f941c134ffc
[scale=32768 Bytes] MD5 time: 228235 ns
Digest: 315a5931d0f93fd1f62a15d77cb234ef
[scale=65536 Bytes] MD5 time: 350592 ns
Digest: 8f1445bafe2c2095044af7789462f475
[scale=131072 Bytes] MD5 time: 794874 ns
Digest: 38a503307786991a982f8ded498b90e0
[scale=262144 Bytes] MD5 time: 1663729 ns
Digest: d19215b1d714757e1fdb0060c52fd4c8
[scale=524288 Bytes] MD5 time: 3210976 ns
Digest: b86919af2c4e30c5e717b40641aa0ef2
[scale=1048576 Bytes] MD5 time: 6523497 ns
Digest: c35cc7d8d91728a0cb052831bc4ef372
~~~



# 主要优化点

## padding 计算

该优化将原本通过循环逐步递增 `new_len` 直到满足 `new_len % 64 == 56` 的 padding 计算方式，改为利用 MD5 以 64 字节为对齐单位的特性，通过 `rem = new_len & 63U` 快速得到当前偏移，并用常数公式直接计算需要补齐的字节数，从而将原先可能存在多次循环与取模运算的过程转化为常数时间计算。该优化消除了循环开销与取模操作，提高了 padding 阶段的执行效率和稳定性。

优化前：md5.cpp

```c
for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++);
```

优化后：md5.cpp

```c
new_len = initial_len + 1;
size_t rem = new_len & 63U;
size_t pad = (rem <= 56U) ? (56U - rem) : (120U - rem);
new_len += pad;
```



~~~
[dongliang@openEuler crypt]$ MD5_PAD_OPT=1 MD5_ALIGN_OPT=0 MD5_PREFETCH_OPT=0 MD5_UNROLL_OPT=0 ./md5_aba.sh
MD5 flags: -DMD5_PAD_OPT=1 -DMD5_ALIGN_OPT=0 -DMD5_PREFETCH_OPT=0 -DMD5_UNROLL_OPT=0
[scale=128 Bytes] MD5 time: 7563 ns
Digest: 37eff01866ba3f538421b30b7cbefcac
[scale=256 Bytes] MD5 time: 2188 ns
Digest: e2c865db4162bed963bfaa9ef6ac18f0
[scale=512 Bytes] MD5 time: 3417 ns
Digest: f5c8e3c31c044bae0e65569560b54332
[scale=1024 Bytes] MD5 time: 9271 ns
Digest: b2ea9f7fcea831a4a63b213f41a8855b
[scale=2048 Bytes] MD5 time: 15584 ns
Digest: 1576a94d6cb334dd126cb1c27f19e0f2
[scale=4096 Bytes] MD5 time: 22688 ns
Digest: 2bcd3c4de20c918e19fab5c36249c70d
[scale=8192 Bytes] MD5 time: 51877 ns
Digest: 6556112372898c69e1de0bf689d8db26
[scale=16384 Bytes] MD5 time: 88148 ns
Digest: 3df67097cee5e4cea36e0f941c134ffc
[scale=32768 Bytes] MD5 time: 227255 ns
Digest: 315a5931d0f93fd1f62a15d77cb234ef
[scale=65536 Bytes] MD5 time: 348966 ns
Digest: 8f1445bafe2c2095044af7789462f475
[scale=131072 Bytes] MD5 time: 780393 ns
Digest: 38a503307786991a982f8ded498b90e0
[scale=262144 Bytes] MD5 time: 1533223 ns
Digest: d19215b1d714757e1fdb0060c52fd4c8
[scale=524288 Bytes] MD5 time: 3099008 ns
Digest: b86919af2c4e30c5e717b40641aa0ef2
[scale=1048576 Bytes] MD5 time: 6354875 ns
Digest: c35cc7d8d91728a0cb052831bc4ef372
~~~



## 预分配/对齐与复用（AArch64 优化）

该优化在原有仅使用 `malloc` 进行内存分配的基础上，引入了按 64 字节对齐的 `posix_memalign` 分配方式（在 AArch64 条件下），以提高数据访问的 cache 对齐友好性，从而减少可能的非对齐访问开销；同时保留 `malloc` 作为失败回退机制，保证分配的健壮性与兼容性，并通过复用 `msg_size` 避免重复分配，减少动态内存管理带来的额外开销。整体上，该优化主要通过提升内存对齐质量与减少无效分配操作，提高后续 MD5 chunk 处理阶段的数据访问效率。

优化前仅 `malloc`：md5.cpp

```c
if (msg_size < new_len + 8){
    free(msg);
    msg = (uint8_t*)malloc(new_len + 8);
    msg_size = new_len + 8;
}
```

优化后对齐分配 + 复用失败回退：md5.cpp

```c
if (msg_size < new_len + 8){
    free(msg);
    msg = NULL;
    msg_size = 0;
#if MD5_AARCH64_OPT
    if (posix_memalign((void **)&msg, 64, new_len + 8) != 0) {
        msg = (uint8_t*)malloc(new_len + 8);
    }
#else
    msg = (uint8_t*)malloc(new_len + 8);
#endif
    if (msg != NULL) {
        msg_size = new_len + 8;
    }
}
```



~~~
[dongliang@openEuler crypt]$ MD5_PAD_OPT=0 MD5_ALIGN_OPT=1 MD5_PREFETCH_OPT=0 MD5_UNROLL_OPT=0 ./md5_aba.sh
MD5 flags: -DMD5_PAD_OPT=0 -DMD5_ALIGN_OPT=1 -DMD5_PREFETCH_OPT=0 -DMD5_UNROLL_OPT=0
[scale=128 Bytes] MD5 time: 9438 ns
Digest: 37eff01866ba3f538421b30b7cbefcac
[scale=256 Bytes] MD5 time: 2625 ns
Digest: e2c865db4162bed963bfaa9ef6ac18f0
[scale=512 Bytes] MD5 time: 3812 ns
Digest: f5c8e3c31c044bae0e65569560b54332
[scale=1024 Bytes] MD5 time: 6875 ns
Digest: b2ea9f7fcea831a4a63b213f41a8855b
[scale=2048 Bytes] MD5 time: 15709 ns
Digest: 1576a94d6cb334dd126cb1c27f19e0f2
[scale=4096 Bytes] MD5 time: 28792 ns
Digest: 2bcd3c4de20c918e19fab5c36249c70d
[scale=8192 Bytes] MD5 time: 46251 ns
Digest: 6556112372898c69e1de0bf689d8db26
[scale=16384 Bytes] MD5 time: 119190 ns
Digest: 3df67097cee5e4cea36e0f941c134ffc
[scale=32768 Bytes] MD5 time: 202754 ns
Digest: 315a5931d0f93fd1f62a15d77cb234ef
[scale=65536 Bytes] MD5 time: 446135 ns
Digest: 8f1445bafe2c2095044af7789462f475
[scale=131072 Bytes] MD5 time: 794787 ns
Digest: 38a503307786991a982f8ded498b90e0
[scale=262144 Bytes] MD5 time: 1570887 ns
Digest: d19215b1d714757e1fdb0060c52fd4c8
[scale=524288 Bytes] MD5 time: 3172672 ns
Digest: b86919af2c4e30c5e717b40641aa0ef2
[scale=1048576 Bytes] MD5 time: 6369572 ns
Digest: c35cc7d8d91728a0cb052831bc4ef372
[dongliang@openEuler crypt]$
~~~



## chunk 处理时的预取与对齐提示

该优化在原有直接通过指针访问 `msg + offset` 的基础上，引入了预取（`__builtin_prefetch`）与对齐假设（`__builtin_assume_aligned`）机制。在遍历 MD5 512-bit 分组时，提前预取下一块数据到缓存中，以降低内存访问延迟，同时通过告知编译器数据地址已按 16 字节对齐，使其能够生成更高效的加载指令并减少潜在的非对齐访问开销。整体优化主要通过提升数据局部性与减少访存延迟，提高 chunk 处理阶段的执行效率。

优化前直接取指针：md5.cpp

```c
for(offset=0; offset<new_len; offset += (512/8)) {
#ifdef MD5MemCopyOpt
    w = (uint32_t *)(msg + offset);
```

优化后加入预取与对齐假设：md5.cpp

```c
for(offset=0; offset<new_len; offset += (512/8)) {
#if MD5_AARCH64_OPT
    if (offset + (512/8) < new_len) {
        __builtin_prefetch(msg + offset + (512/8), 0, 3);
    }
#endif
#ifdef MD5MemCopyOpt
#if MD5_AARCH64_OPT
    w = (uint32_t *)__builtin_assume_aligned(msg + offset, 16);
#else
    w = (uint32_t *)(msg + offset);
#endif
```



~~~
[dongliang@openEuler crypt]$ MD5_PAD_OPT=0 MD5_ALIGN_OPT=0 MD5_PREFETCH_OPT=1 MD5_UNROLL_OPT=0 ./md5_aba.sh
MD5 flags: -DMD5_PAD_OPT=0 -DMD5_ALIGN_OPT=0 -DMD5_PREFETCH_OPT=1 -DMD5_UNROLL_OPT=0
[scale=128 Bytes] MD5 time: 7375 ns
Digest: 37eff01866ba3f538421b30b7cbefcac
[scale=256 Bytes] MD5 time: 2646 ns
Digest: e2c865db4162bed963bfaa9ef6ac18f0
[scale=512 Bytes] MD5 time: 3500 ns
Digest: f5c8e3c31c044bae0e65569560b54332
[scale=1024 Bytes] MD5 time: 9208 ns
Digest: b2ea9f7fcea831a4a63b213f41a8855b
[scale=2048 Bytes] MD5 time: 15667 ns
Digest: 1576a94d6cb334dd126cb1c27f19e0f2
[scale=4096 Bytes] MD5 time: 22751 ns
Digest: 2bcd3c4de20c918e19fab5c36249c70d
[scale=8192 Bytes] MD5 time: 63501 ns
Digest: 6556112372898c69e1de0bf689d8db26
[scale=16384 Bytes] MD5 time: 106273 ns
Digest: 3df67097cee5e4cea36e0f941c134ffc
[scale=32768 Bytes] MD5 time: 345132 ns
Digest: 315a5931d0f93fd1f62a15d77cb234ef
[scale=65536 Bytes] MD5 time: 354216 ns
Digest: 8f1445bafe2c2095044af7789462f475
[scale=131072 Bytes] MD5 time: 778224 ns
Digest: 38a503307786991a982f8ded498b90e0
[scale=262144 Bytes] MD5 time: 1613720 ns
Digest: d19215b1d714757e1fdb0060c52fd4c8
[scale=524288 Bytes] MD5 time: 3275211 ns
Digest: b86919af2c4e30c5e717b40641aa0ef2
[scale=1048576 Bytes] MD5 time: 6590318 ns
Digest: c35cc7d8d91728a0cb052831bc4ef372
[dongliang@openEuler crypt]$
~~~

## 主轮次循环（baseline 64 次循环 vs optimized AArch64 全展开）

优化前的 MD5 主循环是“统一 64 次迭代 + 分支判断 + 取模运算”，每一轮都要进行 `if-else` 判断来决定函数 F/G/H/I 和索引 `g`，同时 `%16` 取模也有额外开销；优化后在 AArch64 路径中将 64 轮完全展开成固定的 `MD5_STEP` 序列，把四个阶段的逻辑提前写死，消除了循环分支、条件跳转和取模运算，同时让编译器更容易做寄存器分配与指令调度，从而显著提升指令级并行度和流水线利用率。

优化前：md5.cpp

```c
for(i = 0; i<64; i++) {
    if (i < 16) {
        f = (b & c) | ((~b) & d);
        g = i;
    } else if (i < 32) {
        f = (d & b) | ((~d) & c);
        g = (5*i + 1) % 16;
    } else if (i < 48) {
        f = b ^ c ^ d;
        g = (3*i + 5) % 16;
    } else {
        f = c ^ (b | (~d));
        g = (7*i) % 16;
    }

    temp = d;
    d = c;
    c = b;
    b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
    a = temp;
}
```

优化后（AArch64 路径全展开）：md5.cpp

```c
// Main loop:
#if MD5_AARCH64_OPT
        // Fully unrolled rounds for AArch64 to avoid branches and modulo ops.
        MD5_STEP(MD5_F, a, b, c, d, w[0], 0xd76aa478, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[1], 0xe8c7b756, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[2], 0x242070db, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[3], 0xc1bdceee, 22);
        MD5_STEP(MD5_F, a, b, c, d, w[4], 0xf57c0faf, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[5], 0x4787c62a, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[6], 0xa8304613, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[7], 0xfd469501, 22);
        MD5_STEP(MD5_F, a, b, c, d, w[8], 0x698098d8, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[9], 0x8b44f7af, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[10], 0xffff5bb1, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[11], 0x895cd7be, 22);
        MD5_STEP(MD5_F, a, b, c, d, w[12], 0x6b901122, 7);
        MD5_STEP(MD5_F, d, a, b, c, w[13], 0xfd987193, 12);
        MD5_STEP(MD5_F, c, d, a, b, w[14], 0xa679438e, 17);
        MD5_STEP(MD5_F, b, c, d, a, w[15], 0x49b40821, 22);

        MD5_STEP(MD5_G, a, b, c, d, w[1], 0xf61e2562, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[6], 0xc040b340, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[11], 0x265e5a51, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[0], 0xe9b6c7aa, 20);
        MD5_STEP(MD5_G, a, b, c, d, w[5], 0xd62f105d, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[10], 0x02441453, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[15], 0xd8a1e681, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[4], 0xe7d3fbc8, 20);
        MD5_STEP(MD5_G, a, b, c, d, w[9], 0x21e1cde6, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[14], 0xc33707d6, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[3], 0xf4d50d87, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[8], 0x455a14ed, 20);
        MD5_STEP(MD5_G, a, b, c, d, w[13], 0xa9e3e905, 5);
        MD5_STEP(MD5_G, d, a, b, c, w[2], 0xfcefa3f8, 9);
        MD5_STEP(MD5_G, c, d, a, b, w[7], 0x676f02d9, 14);
        MD5_STEP(MD5_G, b, c, d, a, w[12], 0x8d2a4c8a, 20);

        MD5_STEP(MD5_H, a, b, c, d, w[5], 0xfffa3942, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[8], 0x8771f681, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[11], 0x6d9d6122, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[14], 0xfde5380c, 23);
        MD5_STEP(MD5_H, a, b, c, d, w[1], 0xa4beea44, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[4], 0x4bdecfa9, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[7], 0xf6bb4b60, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[10], 0xbebfbc70, 23);
        MD5_STEP(MD5_H, a, b, c, d, w[13], 0x289b7ec6, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[0], 0xeaa127fa, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[3], 0xd4ef3085, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[6], 0x04881d05, 23);
        MD5_STEP(MD5_H, a, b, c, d, w[9], 0xd9d4d039, 4);
        MD5_STEP(MD5_H, d, a, b, c, w[12], 0xe6db99e5, 11);
        MD5_STEP(MD5_H, c, d, a, b, w[15], 0x1fa27cf8, 16);
        MD5_STEP(MD5_H, b, c, d, a, w[2], 0xc4ac5665, 23);

        MD5_STEP(MD5_I, a, b, c, d, w[0], 0xf4292244, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[7], 0x432aff97, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[14], 0xab9423a7, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[5], 0xfc93a039, 21);
        MD5_STEP(MD5_I, a, b, c, d, w[12], 0x655b59c3, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[3], 0x8f0ccc92, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[10], 0xffeff47d, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[1], 0x85845dd1, 21);
        MD5_STEP(MD5_I, a, b, c, d, w[8], 0x6fa87e4f, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[15], 0xfe2ce6e0, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[6], 0xa3014314, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[13], 0x4e0811a1, 21);
        MD5_STEP(MD5_I, a, b, c, d, w[4], 0xf7537e82, 6);
        MD5_STEP(MD5_I, d, a, b, c, w[11], 0xbd3af235, 10);
        MD5_STEP(MD5_I, c, d, a, b, w[2], 0x2ad7d2bb, 15);
        MD5_STEP(MD5_I, b, c, d, a, w[9], 0xeb86d391, 21);
#else
        for(i = 0; i<64; i++) {

            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }

            temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;

        }
#endif
```



~~~
[dongliang@openEuler crypt]$ MD5_PAD_OPT=0 MD5_ALIGN_OPT=0 MD5_PREFETCH_OPT=0 MD5_UNROLL_OPT=1 ./md5_aba.sh
MD5 flags: -DMD5_PAD_OPT=0 -DMD5_ALIGN_OPT=0 -DMD5_PREFETCH_OPT=0 -DMD5_UNROLL_OPT=1
[scale=128 Bytes] MD5 time: 14063 ns
Digest: 37eff01866ba3f538421b30b7cbefcac
[scale=256 Bytes] MD5 time: 12292 ns
Digest: e2c865db4162bed963bfaa9ef6ac18f0
[scale=512 Bytes] MD5 time: 2626 ns
Digest: f5c8e3c31c044bae0e65569560b54332
[scale=1024 Bytes] MD5 time: 8021 ns
Digest: b2ea9f7fcea831a4a63b213f41a8855b
[scale=2048 Bytes] MD5 time: 12376 ns
Digest: 1576a94d6cb334dd126cb1c27f19e0f2
[scale=4096 Bytes] MD5 time: 17104 ns
Digest: 2bcd3c4de20c918e19fab5c36249c70d
[scale=8192 Bytes] MD5 time: 37813 ns
Digest: 6556112372898c69e1de0bf689d8db26
[scale=16384 Bytes] MD5 time: 67335 ns
Digest: 3df67097cee5e4cea36e0f941c134ffc
[scale=32768 Bytes] MD5 time: 167483 ns
Digest: 315a5931d0f93fd1f62a15d77cb234ef
[scale=65536 Bytes] MD5 time: 262150 ns
Digest: 8f1445bafe2c2095044af7789462f475
[scale=131072 Bytes] MD5 time: 628553 ns
Digest: 38a503307786991a982f8ded498b90e0
[scale=262144 Bytes] MD5 time: 1233418 ns
Digest: d19215b1d714757e1fdb0060c52fd4c8
[scale=524288 Bytes] MD5 time: 2427501 ns
Digest: b86919af2c4e30c5e717b40641aa0ef2
[scale=1048576 Bytes] MD5 time: 4942816 ns
Digest: c35cc7d8d91728a0cb052831bc4ef372
[dongliang@openEuler crypt]$
~~~



| **数据规模 (Bytes)** | **Baseline** | **最终优化 (Final)** | **Padding 优化** | **对齐复用优化** | **预取提示优化** | **循环全展开优化** |
| -------------------- | ------------ | -------------------- | ---------------- | ---------------- | ---------------- | ------------------ |
| **128**              | 6937         | 14205                | 7563             | 9438             | 7375             | 14063              |
| **256**              | 2333         | 12390                | 2188             | 2625             | 2646             | 12292              |
| **512**              | 3417         | 2816                 | 3417             | 3812             | 3500             | 2626               |
| **1024 (1K)**        | 8958         | 5090                 | 9271             | 6875             | 9208             | 8021               |
| **2048 (2K)**        | 14833        | 12306                | 15584            | 15709            | 15667            | 12376              |
| **4096 (4K)**        | 22667        | 20545                | 22688            | 28792            | 22751            | 17104              |
| **8192 (8K)**        | 81773        | 34417                | 51877            | 46251            | 63501            | 37813              |
| **16384 (16K)**      | 94752        | 77573                | 88148            | 119190           | 106273           | 67335              |
| **32768 (32K)**      | 228235       | 151912               | 227255           | 202754           | 345132           | 167483             |
| **65536 (64K)**      | 350592       | 350361               | 348966           | 446135           | 354216           | 262150             |
| **131072 (128K)**    | 794874       | 661695               | 780393           | 794787           | 778224           | 628553             |
| **262144 (256K)**    | 1663729      | 1201409              | 1533223          | 1570887          | 1613720          | 1233418            |
| **524288 (512K)**    | 3210976      | 2430351              | 3099008          | 3172672          | 3275211          | 2427501            |
| **1048576 (1M)**     | 6523497      | 4855467              | 6354875          | 6369572          | 6590318          | 4942816            |

+ 时间单位为ns