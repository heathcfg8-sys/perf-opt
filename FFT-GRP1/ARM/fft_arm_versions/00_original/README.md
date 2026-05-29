# 00_original_formatted_baseline

原始格式化基线：保留原始 FFT 计算流程，不额外加入本次三种优化。

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
