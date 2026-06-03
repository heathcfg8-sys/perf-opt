# CRYPT-GRP3 — 密码学算子性能优化

对 AES-128/192/256、SHA-256 和 MD5 三种密码学算子在嵌入式平台上进行性能优化，通过多种编译期与运行时优化手段降低执行时间。

---

## 目录结构

```
CRYPT-GRP3/
├── aes/                    # AES 加解密实现（128/192/256, ECB模式）
│   ├── aes_interface/      # 基准测试框架（14种数据规模: 128B ~ 1MiB）
│   ├── Record.md           # 优化记录与性能数据
│   └── 运行.md             # 本地编译运行说明
├── sha256/
│   ├── sha256_baseline/    # SHA-256 基准实现
│   ├── sha256_optimized/   # SHA-256 优化实现
│   └── tda4-a72-linux.md   # 编译运行说明
├── md5/
│   ├── md5_baseline/       # MD5 基准实现
│   ├── md5_optimized/      # MD5 完整优化实现
│   ├── md5_ablation/       # MD5 消融实验（可独立开关各优化）
│   ├── md5_test/           # 测试框架与计时工具
│   ├── *.sh                # 编译运行脚本
│   └── MD5_README.md       # 优化详细说明
├── param.h                 # 全局编译期优化开关
├── CMakeLists.txt          # TDA4 交叉编译（aarch64）
└── tda4-a72-linux.md       # 构建与运行指南
```

## 目标平台

- **SoC**: TI TDA4 (Jacinto, ARM Cortex-A72)
- **架构**: AArch64 / armv8-a
- **编译器**: `aarch64-linux-gnu-gcc` / `aarch64-linux-gnu-g++`
- **构建**: CMake, 静态链接

## 优化总览

### AES

| 优化 | 方法 |
|------|------|
| ShiftRows 展开 | 用硬编码赋值替代循环移位 |
| SubBytes 展开 | 单层循环→完全展开16路S盒查表 |
| AddRoundKey 展开 | 预计算轮密钥指针，展开XOR循环 |
| InvMixColumns 内联 | 消除临时数组，常量替代查表 |
| **总收益** | 1 MiB数据: ~298ms → ~182ms (**~39%**) |

### SHA-256

| 优化 | 方法 |
|------|------|
| 去除冗余掩码 | 消除 `& 0xff` / `& 0xffffffff` |
| 分支预测提示 | `__builtin_expect` |
| Padding 优化 | 手动memset循环 → `memset()` |
| 数据预取 | `__builtin_prefetch` |
| 压缩函数展开 | 宏定义 + 64轮手动展开为16×4轮 |

### MD5

| 优化 | 方法 |
|------|------|
| Padding 计算 | 循环求余 → O(1) 位运算 |
| 对齐分配 + 缓存复用 | `posix_memalign(64)` + 静态缓冲区 |
| 数据预取 + 对齐提示 | `__builtin_prefetch` + `__builtin_assume_aligned` |
| 主循环完全展开 | 全部64轮宏展开，消除分支与取模 |
| **总收益** | 1 MiB数据: 6.52ms → 4.86ms (**1.34x**) |


## 基准测试

各组件均对 128B ~ 1MiB 共 14 种数据规模进行性能测试，输出每个规模下的单次执行时间（ns）。详细数据见各子目录下的优化记录文档。


