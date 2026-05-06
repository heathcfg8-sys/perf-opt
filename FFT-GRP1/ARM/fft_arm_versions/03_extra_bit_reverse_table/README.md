# 03_extra_bit_reverse_table_cache

文档外方法：位反转索引表缓存。提前生成并缓存 bit-reversal 表，运行时查表完成置换。

## 优化使用位置

本版本的优化主要集中在 `fft_a.c`。

1. `fft_a.c` 文件开头新增了位反转缓存结构体和全局缓存对象：

```c
typedef struct BitReverseCache_s {
    int size;
    int level_count;
    uint32_t *indices;
} BitReverseCache_t;

static BitReverseCache_t g_bit_reverse_cache = {0};
```

2. `prepare_bit_reverse_cache()` 函数负责提前生成位反转索引表。核心逻辑是把每个下标 `i` 的位反转结果预先算好并存入 `indices`：

```c
g_bit_reverse_cache.indices[i] = reverse_bits((uint32_t)i, level_count);
```

3. `bit_reverse_reorder()` 函数中不再每次动态计算反转下标，而是直接查表：

```c
const uint32_t *indices = g_bit_reverse_cache.indices;
int reversed_index = (int)indices[i];
```

4. `fft_linux_iopointer()` 和 `fft_linux_ioself_profiling()` 在执行 FFT 前会调用 `prepare_bit_reverse_cache(size)`，保证对应规模的位反转表已经准备好。

简而言之，原始版本在每次 FFT 位反转重排时都需要在循环中计算反转下标；本版本把这部分整数计算提前缓存，运行时只查表，从而减少热点流程中的控制流和整数运算开销。

## 编译与运行

ARM64/QEMU：

```bash
make arm
qemu-aarch64 -cpu max ./fft
```

本机功能检查：

```bash
make native
./fft
```

外部输入指针接口测试：

```bash
make call_arm
qemu-aarch64 -cpu max ./call_test
```
