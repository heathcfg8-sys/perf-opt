
# MPC-GRP3 研讨代码说明

## 一、程序概述

本程序实现了一个基于模型预测控制（MPC）的车辆轨迹跟踪控制器。

程序通过优化未来的控制序列，包括方向盘转角 `delta` 和加速度 `a`，使车辆尽量跟踪下面这条参考路径：

```text
y = sin(0.2x) + 0.1x
```

经过试验后，主要使用到的优化方式包括：

- 轨迹缓存复用：避免重复模拟，梯度计算从 O(N²) 降到 O(N)
- 预计算参考路径表：用插值查表代替运行时反复计算
- NEON SIMD 向量化
- OpenMP 并行计算
- 线程局部缓存
- 小角度近似优化
- 循环展开

其中对性能影响比较明显的是轨迹缓存复用和参考路径表预计算。

## 二、文件结构

```text
├── mpc_linux.h          # 头文件
├── mpc_linux.c          # 核心实现
├── mpc_linux_test.c     # 测试程序
├── timestamp.h          # 时间戳头文件
├── timestamp.c          # 时间戳实现
├── README.md            # 说明文档
└── 研讨报告.pptx        # 汇报PPT
```

## 三、编译方法

### 环境要求

本程序主要在香橙派上的 Linux 系统和 ARM 架构下运行。

依赖库：

```text
libomp-dev
```

### 基础编译

```bash
gcc -o mpc_test mpc_linux_test.c mpc_linux.c timestamp.c \
    -O3 -march=native -fopenmp -lm -Wall
```

### 启用详细输出模式

调试时可以加上 `-DMPC_VERBOSE=1`：

```bash
gcc -o mpc_test mpc_linux_test.c mpc_linux.c timestamp.c \
    -O3 -march=native -fopenmp -lm -Wall -DMPC_VERBOSE=1
```

### 分步编译

```bash
gcc -c -O3 -march=native -fopenmp -Wall mpc_linux.c -o mpc_linux.o
gcc -c -O3 -march=native -fopenmp -Wall mpc_linux_test.c -o mpc_linux_test.o
gcc -c -O3 -march=native -Wall timestamp.c -o timestamp.o
gcc -o mpc_test mpc_linux.o mpc_linux_test.o timestamp.o -fopenmp -lm
```

## 四、执行方法

### 单一步长测试

```bash
./mpc_test 39
```

### 多个步长对比测试

```bash
./mpc_test 39 78 156
```

### 查看帮助

```bash
./mpc_test
```

### 设置 OpenMP 线程数

```bash
export OMP_NUM_THREADS=4
./mpc_test 39
```

代码中目前默认设置为 2 个线程。

## 五、运行示例

```text
$ ./mpc_test 39

Thread 0 / 2
Thread 1 / 2
==== Testing horizon=39 (cached) ====
Initial State: x=12.345, y=3.456, psi=0.789, v=8.234, cte=0.123, epsi=0.045
Average CTE over horizon: 0.234
Control 0: delta=0.123, a=0.456
Control 1: delta=0.234, a=0.567
Control 2: delta=0.189, a=0.432
...
Control 38: delta=0.156, a=0.321
mpc_linux_ioself_profiling_cached: horizon=39 total_time=15234567 ns 15.235 ms
```

多个 horizon 测试时会继续输出后续结果。

## 六、接口函数说明

### `RetCode_t mpc_linux_iopointer(int horizon, void* input, void* output)`

执行一次 MPC 计算，输出最优控制序列。

参数：

- `horizon`：预测步长
- `input`：指向 `State_t` 的指针
- `output`：指向 `Controls_t` 的指针

返回值：

- `K_RET_OK`：成功
- 其他值：错误码

### `RetCode_t mpc_linux_ioself_profiling(int horizon)`

用于性能测试，函数内部会随机生成状态并计时。

参数：

- `horizon`：预测步长

返回值：

- `K_RET_OK`：成功

### `RetCode_t mpc_linux_iopointer_cached(int horizon, void* input, void* output, MPCThreadCache_t* cache)`

带缓存的 MPC 计算接口。通过复用缓存，减少重复内存分配带来的开销。

参数：

- `horizon`：预测步长
- `input`：指向 `State_t` 的指针
- `output`：指向 `Controls_t` 的指针
- `cache`：线程局部缓存结构

返回值：

- `K_RET_OK`：成功

### `void mpc_cache_init(MPCThreadCache_t* cache, int max_horizon)`

初始化线程缓存。

参数：

- `cache`：缓存结构指针
- `max_horizon`：最大预测步长

### `void mpc_cache_free(MPCThreadCache_t* cache)`

释放线程缓存内存。

参数：

- `cache`：缓存结构指针

## 七、参数配置说明

主要可调参数位于 `mpc_linux.c` 文件头部，和原程序保持一致。

例如：

```c
#define MPC_VERBOSE 0
```

将其设置为 `1` 可以启用详细输出：

```c
#define MPC_VERBOSE 1
```