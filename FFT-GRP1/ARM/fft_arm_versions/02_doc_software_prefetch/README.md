# 02_doc_software_prefetch

文档内方法：软件预取。热点蝶形循环中使用 `__builtin_prefetch()` 预取后续数据和旋转因子。

## 优化使用位置

本版本的优化主要集中在 `fft_a.c` 的 `fft_execute()` 函数。

1. `fft_a.c` 文件开头定义了预取距离：

```c
#define FFT_PREFETCH_DISTANCE 16
```

2. 在 `fft_execute()` 的标量蝶形循环中，当前批次计算前会预取后续蝶形要访问的数据：

```c
__builtin_prefetch(x_real + next_left_index, 0, 1);
__builtin_prefetch(x_imag + next_left_index, 0, 1);
__builtin_prefetch(x_real + next_right_index, 0, 1);
__builtin_prefetch(x_imag + next_right_index, 0, 1);
__builtin_prefetch(table->real[level] + k + FFT_PREFETCH_DISTANCE, 0, 1);
__builtin_prefetch(table->imag[level] + k + FFT_PREFETCH_DISTANCE, 0, 1);
```

3. 在 `fft_execute()` 的 NEON 向量蝶形循环中，也加入了同样的预取逻辑，用于提前加载后续的 `x_real`、`x_imag` 和旋转因子表。

4. `__builtin_prefetch(ptr, 0, 1)` 中，第二个参数 `0` 表示读预取，第三个参数 `1` 表示较低局部性提示。

简而言之，本版本没有改变 FFT 计算结果，而是在蝶形计算真正访问后续数据前，提前向 Cache 发出预取提示，尝试降低后续访存等待。

需要注意：软件预取依赖真实硬件的 Cache、预取器和访存延迟。在 QEMU AArch64 模拟环境下，它可能没有明显收益，甚至可能因为额外预取指令和判断开销导致运行时间变长。

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
