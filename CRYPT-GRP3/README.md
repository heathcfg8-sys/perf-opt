# CRYPT-GRP3 — 密码学算子性能优化

对 AES-128/192/256、SHA-256 和 MD5 三种密码学算子在嵌入式平台（TI TDA4, Cortex-A72）上进行性能优化，通过多种编译期与运行时优化手段降低执行时间。

---

## 目录结构

```
CRYPT-GRP3/
├── aes/                        # AES 加解密实现（128/192/256, ECB模式）
│   ├── aes_interface/          # 完整实现 + 基准测试框架
│   ├── Record.md               # 优化记录与性能数据
│   ├── build.sh                # 本地编译运行脚本（g++）
│   └── 运行.md                 # 本地编译运行说明
├── sha256/
│   ├── sha256_baseline/        # SHA-256 基准实现
│   │   └── sha256/             # 源文件
│   ├── sha256_optimized/       # SHA-256 优化实现
│   │   └── sha256/             # 源文件
│   ├── sha256_baseline.sh      # 本地编译运行脚本（baseline）
│   └── sha256_optimized.sh     # 本地编译运行脚本（optimized）
├── md5/
│   ├── md5_baseline/           # MD5 基准实现
│   ├── md5_optimized/          # MD5 完整优化实现
│   ├── md5_ablation/           # MD5 消融实验（可独立开关各优化）
│   ├── md5_test/               # 测试框架与计时工具
│   ├── md5_baseline.sh         # 本地编译运行脚本（baseline）
│   ├── md5_optimized.sh        # 本地编译运行脚本（optimized）
│   ├── md5_ablation.sh         # 本地编译运行脚本（ablation）
│   └── MD5_README.md           # 优化详细说明
├── param.h                     # 全局编译期优化开关
├── CMakeLists.txt              # TDA4 交叉编译（aarch64）
├── tda4-a72-linux.md           # 构建与运行指南
└── README.md                   # 本文件
```

---
## 目标平台

- **SoC**: TI TDA4 (Jacinto, ARM Cortex-A72)
- **架构**: AArch64 / armv8-a
- **编译器**: `aarch64-linux-gnu-gcc` / `aarch64-linux-gnu-g++`（交叉编译）
- **本地开发**: `gcc` / `g++`（x86_64 验证）

---

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

---

## 编译 & 运行

### 本地开发（g++，无需 cmake）

各组件均提供独立 bash 脚本，直接运行即可：

```bash
# MD5
cd md5 && bash md5_baseline.sh
cd md5 && bash md5_optimized.sh
cd md5 && bash md5_ablation.sh     # 支持环境变量开关优化

# SHA-256
cd sha256 && bash sha256_baseline.sh
cd sha256 && bash sha256_optimized.sh

# AES
cd aes && bash build.sh
```

MD5 消融实验支持环境变量独立开关各项优化：

```bash
MD5_PAD_OPT=1 MD5_ALIGN_OPT=1 MD5_PREFETCH_OPT=1 MD5_UNROLL_OPT=0 bash md5_ablation.sh
```

### TDA4 交叉编译（CMake）

```bash
mkdir build && cd build
cmake ..
make
```

环境要求：`cmake` + `aarch64-linux-gnu-gcc` 交叉编译器。

生成 `aes_encrypt`、`md5_hash`、`sha256_hash` 三个可执行文件，scp 到 TDA4 上运行。

### 优化开关

编辑 `param.h` 中的宏定义以开关各组件的优化：

| 宏 | 作用 |
|---|------|
| `MD5MemCopyOpt` | MD5 字级内存拷贝 |
| `MD5MallocOpt` | MD5 静态缓冲复用 |
| `MD5LoopUnroll` | MD5 主循环展开 |
| `SHA256RremoveUselessCal` | SHA-256 去除冗余掩码 |
| `SHA256BranchPred` | SHA-256 分支预测提示 |
| `SHA256PaddingOpt` | SHA-256 Padding 优化 |
| `SHA256PrefetchOpt` | SHA-256 数据预取 |
| `SHA256UnrollOpt` | SHA-256 压缩函数展开 |
| `AESMergeShift` | AES 合并 ShiftRows |

---

## 基准测试

各组件均对 128B ~ 1MiB 共 14 种数据规模进行性能测试：

- **MD5**: 输出纳秒耗时 + 16字节摘要（三个变体摘要一致）
- **SHA-256**: 输出纳秒/毫秒耗时
- **AES**: 输出毫秒耗时 + 加密-解密一致性校验

详细数据见各子目录下的优化记录文档。

