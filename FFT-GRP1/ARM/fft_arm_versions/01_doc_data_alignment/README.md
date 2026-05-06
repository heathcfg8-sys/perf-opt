# 01_doc_data_alignment_64byte

文档内方法：数据对齐。所有工作缓冲区和旋转因子数组使用 64 字节对齐分配。

## 优化使用位置

本版本的优化主要集中在 `fft_a.c`。

1. `fft_a.c` 文件开头定义了对齐大小：

```c
#define FFT_ALIGNMENT 64U
```

2. `fft_a.c` 中的 `fft_malloc()` 函数统一负责对齐分配。该函数先把申请字节数补齐到 64 字节倍数，再调用 `aligned_alloc()`：

```c
aligned_size = (size + FFT_ALIGNMENT - 1U) & ~(FFT_ALIGNMENT - 1U);
return aligned_alloc(FFT_ALIGNMENT, aligned_size);
```

3. 旋转因子数组在 `build_twiddle_table()` 中使用 `fft_malloc()` 分配，因此 `table->real[level]` 和 `table->imag[level]` 是 64 字节对齐的。

4. 测试输入缓冲区在 `fft_linux_ioself_profiling()` 和 `fft_linux_ioself()` 中使用 `fft_malloc()` 分配，因此 `input_real`、`input_imag`、`work_real`、`work_imag` 也是 64 字节对齐的。

简而言之，本版本不是改变 FFT 算法流程，而是把热点计算用到的主要数组统一改成 64 字节对齐，减少非对齐访问对 ARM Cache 和 SIMD 读取的影响。

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
