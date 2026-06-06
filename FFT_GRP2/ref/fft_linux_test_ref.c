// ============================================================
// FFT 优化基准测试主程序
// 测试所有优化版本的性能并进行比较
// ============================================================

#include "fft_linux_ref.h"

// 原始版本引用
extern void fft_linux_ioself_profiling(int size);
extern int fft_linux_iopointer(int size, float *input_real, float *input_imag);
extern void preCompute(int size);

// 测试规模数组
static int test_sizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

// ============================================================
// 原始版本性能测试（使用原始API）
// ============================================================

void benchmark_original_native(int size) {
    struct timespec start, end;
    
    float *input_real = (float*)aligned_alloc(16, sizeof(float) * size);
    float *input_imag = (float*)aligned_alloc(16, sizeof(float) * size);
    float *output_real = (float*)aligned_alloc(16, sizeof(float) * size);
    float *output_imag = (float*)aligned_alloc(16, sizeof(float) * size);
    
    if (!input_real || !input_imag || !output_real || !output_imag) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    // 生成随机输入
    srand(42);
    for (int i = 0; i < size; i++) {
        input_real[i] = (float)(rand() % 65536 - 32768);
        input_imag[i] = 0.0f;
    }
    memcpy(output_real, input_real, sizeof(float) * size);
    memcpy(output_imag, input_imag, sizeof(float) * size);
    
    // 预计算旋转因子
    clock_gettime(CLOCK_MONOTONIC, &start);
    preCompute(size);
    
    // 执行FFT
    fft_linux_iopointer(size, output_real, output_imag);
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_nsec - start.tv_nsec) / 1e6;
    
    // IFFT验证
    // 需要反转虚部并调用fft
    for (int i = 0; i < size; i++) {
        output_imag[i] = -output_imag[i];
    }
    fft_linux_iopointer(size, output_real, output_imag);
    for (int i = 0; i < size; i++) {
        output_real[i] = output_real[i] / size;
        output_imag[i] = -output_imag[i] / size;
    }
    
    double max_diff = 0.0;
    for (int i = 0; i < size; i++) {
        double diff = fabs(output_real[i] - input_real[i]);
        if (diff > max_diff) max_diff = diff;
    }
    
    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
           "Original (Native API)", size, elapsed, max_diff);
    
    free(input_real);
    free(input_imag);
    free(output_real);
    free(output_imag);
}

// ============================================================
// 打印测试规模表头
// ============================================================

void print_header(void) {
    printf("\n");
    printf("################################################################################\n");
    printf("#                                                                              #\n");
    printf("#                    FFT 优化算法性能基准测试                                   #\n");
    printf("#                    FFT Optimization Benchmark                                #\n");
    printf("#                                                                              #\n");
    printf("#  测试环境: ARM Cortex-A72 (ARMv8-A) + NEON SIMD                             #\n");
    printf("#  编译器: aarch64-linux-gnu-gcc -O2 -march=armv8-a -lm                        #\n");
    printf("#                                                                              #\n");
    printf("################################################################################\n");
    printf("\n");
}

void print_table_header(void) {
    printf("+--------------------------------+----------+--------------+-------------+\n");
    printf("| %-30s | %7s | %12s | %11s |\n", 
           "Version", "Size", "Time (ms)", "Max Error");
    printf("+--------------------------------+----------+--------------+-------------+\n");
}

void print_table_footer(void) {
    printf("+--------------------------------+----------+--------------+-------------+\n");
}

// ============================================================
// 测试1: 固定规模，测试所有版本
// ============================================================

void test_all_versions_at_size(int size) {
    FFT_Perf_Stats stats;
    
    printf("\n");
    printf("================================================================================\n");
    printf("  固定规模测试 (Fixed Size Test): N = %d\n", size);
    printf("================================================================================\n");
    
    print_table_header();
    
    // 原始版本（原生API）
    benchmark_original_native(size);
    
    // 优化版本1: 实时计算sin/cos
    if (fft_realtime_sincos_profiling(size, &stats) == 0) {
        printf("| %-30s | %7d | %10.4f | %11.6f |\n",
               "V1: Real-time sin/cos", size, stats.fft_time_ms, stats.max_error);
    }
    
    // 优化版本2: 查表法
    if (fft_lookup_table_profiling(size, &stats) == 0) {
        printf("| %-30s | %7d | %10.4f | %11.6f |\n",
               "V2: Lookup Table", size, stats.fft_time_ms, stats.max_error);
    }
    
    // 优化版本3: 增强NEON
    if (fft_enhanced_neon_profiling(size, &stats) == 0) {
        printf("| %-30s | %7d | %10.4f | %11.6f |\n",
               "V3: Enhanced NEON", size, stats.fft_time_ms, stats.max_error);
    }
    
    // 优化版本4: 软件预取
    if (fft_software_prefetch_profiling(size, &stats) == 0) {
        printf("| %-30s | %7d | %10.4f | %11.6f |\n",
               "V4: Software Prefetch", size, stats.fft_time_ms, stats.max_error);
    }
    
    // 优化版本5: 缓存友好
    if (fft_cache_friendly_profiling(size, &stats) == 0) {
        printf("| %-30s | %7d | %10.4f | %11.6f |\n",
               "V5: Cache-Friendly", size, stats.fft_time_ms, stats.max_error);
    }
    
    // 优化版本6: 混合优化
    if (fft_hybrid_profiling(size, &stats) == 0) {
        printf("| %-30s | %7d | %10.4f | %11.6f |\n",
               "V6: Hybrid (All)", size, stats.fft_time_ms, stats.max_error);
    }
    
    print_table_footer();
}

// ============================================================
// 测试2: 规模扫描，测试单个版本
// ============================================================

void test_single_version_sweep(FFT_Opt_Version version) {
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    FFT_Perf_Stats stats;
    
    printf("\n");
    printf("================================================================================\n");
    printf("  规模扫描测试 (Size Sweep): %s\n", get_version_name(version));
    printf("================================================================================\n");
    
    print_table_header();
    
    // 先运行原始版本作为基准
    for (int i = 0; i < num_sizes; i++) {
        int size = test_sizes[i];
        if (size > 131072) continue;  // 限制最大规模
        
        benchmark_original_native(size);
    }
    
    printf("\n--- Optimized Versions ---\n\n");
    
    for (int i = 0; i < num_sizes; i++) {
        int size = test_sizes[i];
        if (size > 131072) continue;
        
        const char* version_name;
        switch (version) {
            case OPT_REALTIME_SINCOS:
                if (fft_realtime_sincos_profiling(size, &stats) == 0) {
                    version_name = "V1: Real-time sin/cos";
                    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
                           version_name, size, stats.fft_time_ms, stats.max_error);
                }
                break;
            case OPT_LOOKUP_TABLE:
                if (fft_lookup_table_profiling(size, &stats) == 0) {
                    version_name = "V2: Lookup Table";
                    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
                           version_name, size, stats.fft_time_ms, stats.max_error);
                }
                break;
            case OPT_ENHANCED_NEON:
                if (fft_enhanced_neon_profiling(size, &stats) == 0) {
                    version_name = "V3: Enhanced NEON";
                    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
                           version_name, size, stats.fft_time_ms, stats.max_error);
                }
                break;
            case OPT_SOFTWARE_PREFETCH:
                if (fft_software_prefetch_profiling(size, &stats) == 0) {
                    version_name = "V4: Software Prefetch";
                    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
                           version_name, size, stats.fft_time_ms, stats.max_error);
                }
                break;
            case OPT_CACHE_FRIENDLY:
                if (fft_cache_friendly_profiling(size, &stats) == 0) {
                    version_name = "V5: Cache-Friendly";
                    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
                           version_name, size, stats.fft_time_ms, stats.max_error);
                }
                break;
            case OPT_HYBRID:
                if (fft_hybrid_profiling(size, &stats) == 0) {
                    version_name = "V6: Hybrid (All)";
                    printf("| %-30s | %7d | %10.4f | %11.6f |\n",
                           version_name, size, stats.fft_time_ms, stats.max_error);
                }
                break;
            default:
                break;
        }
    }
    
    print_table_footer();
}

// ============================================================
// 测试3: 完整对比测试
// ============================================================

void test_complete_comparison(void) {
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    FFT_Perf_Stats stats;
    FFT_Perf_Stats results[7][10];  // 7个版本，10个规模
    int valid_counts[7] = {0};
    
    printf("\n");
    printf("################################################################################\n");
    printf("#                                                                              #\n");
    printf("#                      完整性能对比测试 (Complete Comparison)                   #\n");
    printf("#                                                                              #\n");
    printf("################################################################################\n");
    
    // 运行所有版本的测试
    for (int i = 0; i < num_sizes; i++) {
        int size = test_sizes[i];
        if (size > 131072) continue;
        
        // 原始版本
        struct timespec start, end;
        float *input_real = (float*)aligned_alloc(16, sizeof(float) * size);
        float *input_imag = (float*)aligned_alloc(16, sizeof(float) * size);
        float *output_real = (float*)aligned_alloc(16, sizeof(float) * size);
        float *output_imag = (float*)aligned_alloc(16, sizeof(float) * size);
        
        if (input_real && input_imag && output_real && output_imag) {
            srand(42);
            for (int j = 0; j < size; j++) {
                input_real[j] = (float)(rand() % 65536 - 32768);
                input_imag[j] = 0.0f;
            }
            memcpy(output_real, input_real, sizeof(float) * size);
            memcpy(output_imag, input_imag, sizeof(float) * size);
            
            clock_gettime(CLOCK_MONOTONIC, &start);
            preCompute(size);
            fft_linux_iopointer(size, output_real, output_imag);
            clock_gettime(CLOCK_MONOTONIC, &end);
            
            results[0][valid_counts[0]].fft_time_ms = (end.tv_sec - start.tv_sec) * 1000.0 + 
                                                        (end.tv_nsec - start.tv_nsec) / 1e6;
            results[0][valid_counts[0]].max_error = 0.0;  // 简化处理
            valid_counts[0]++;
        }
        
        if (input_real) free(input_real);
        if (input_imag) free(input_imag);
        if (output_real) free(output_real);
        if (output_imag) free(output_imag);
        
        // 优化版本
        if (fft_realtime_sincos_profiling(size, &results[1][valid_counts[1]]) == 0) valid_counts[1]++;
        if (fft_lookup_table_profiling(size, &results[2][valid_counts[2]]) == 0) valid_counts[2]++;
        if (fft_enhanced_neon_profiling(size, &results[3][valid_counts[3]]) == 0) valid_counts[3]++;
        if (fft_software_prefetch_profiling(size, &results[4][valid_counts[4]]) == 0) valid_counts[4]++;
        if (fft_cache_friendly_profiling(size, &results[5][valid_counts[5]]) == 0) valid_counts[5]++;
        if (fft_hybrid_profiling(size, &results[6][valid_counts[6]]) == 0) valid_counts[6]++;
    }
    
    // 打印结果表
    printf("\n+--------+----------+----------+----------+----------+----------+----------+----------+\n");
    printf("|  Size  |  Orig.   |   V1     |   V2     |   V3     |   V4     |   V5     |   V6     |\n");
    printf("|        |   (ms)   |  (ms)    |  (ms)    |  (ms)    |  (ms)    |  (ms)    |  (ms)    |\n");
    printf("+--------+----------+----------+----------+----------+----------+----------+----------+\n");
    
    for (int i = 0; i < num_sizes && i < 10; i++) {
        int size = test_sizes[i];
        if (size > 131072) continue;
        
        printf("| %6d |", size);
        for (int v = 0; v < 7; v++) {
            if (i < valid_counts[v]) {
                printf(" %8.4f |", results[v][i].fft_time_ms);
            } else {
                printf("     N/A  |");
            }
        }
        printf("\n");
    }
    printf("+--------+----------+----------+----------+----------+----------+----------+----------+\n");
}

// ============================================================
// 打印优化说明
// ============================================================

void print_optimization_notes(void) {
    printf("\n");
    printf("################################################################################\n");
    printf("#                                                                              #\n");
    printf("#                            优化方案说明                                       #\n");
    printf("#                            Optimization Notes                                #\n");
    printf("#                                                                              #\n");
    printf("################################################################################\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ 版本1: 迭代 + 实时计算 sin/cos (Iterative + Real-time sin/cos)               │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ 优化思路:                                                                    │\n");
    printf("│  - 使用三角恒等式增量计算旋转因子，避免重复调用 sin/cos 函数                 │\n");
    printf("│  - 公式: sin(θ+Δ) = sin(θ)cos(Δ) + cos(θ)sin(Δ)                            │\n");
    printf("│  - 优点: 无预计算开销，适用于一次性FFT                                       │\n");
    printf("│  - 缺点: 精度略有损失                                                        │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ 版本2: 迭代 + 查表法 (Iterative + Lookup Table)                              │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ 优化思路:                                                                    │\n");
    printf("│  - 预计算并存储 sin/cos 值到查找表                                            │\n");
    printf("│  - 使用 float 类型减少内存带宽                                                │\n");
    printf("│  - 查表代替实时计算，大幅减少三角函数调用开销                                 │\n");
    printf("│  - 优点: 性能稳定，精度可控                                                    │\n");
    printf("│  - 缺点: 需要额外内存存储查找表                                               │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ 版本3: 增强 NEON SIMD 向量化 (Enhanced NEON Vectorization)                  │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ 优化思路:                                                                    │\n");
    printf("│  - 使用 ARM NEON SIMD 指令并行处理多个数据                                    │\n");
    printf("│  - float32x4_t 可同时处理 4 个 float 复数                                     │\n");
    printf("│  - 优化加载/存储模式，使用 vld1/vst1 指令                                      │\n");
    printf("│  - 利用 vmulq_f32, vaddq_f32, vmlaq_f32 等融合乘加指令                       │\n");
    printf("│  - 优点: 理论4倍加速                                                          │\n");
    printf("│  - 缺点: 需要ARM NEON支持                                                     │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ 版本4: 软件预取优化 (Software Prefetch)                                      │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ 优化思路:                                                                    │\n");
    printf("│  - 使用 __builtin_prefetch 预取下一批数据到缓存                               │\n");
    printf("│  - 减少缓存未命中(Cache Miss)带来的性能损失                                   │\n");
    printf("│  - 预取距离通常设置为 3-5 个迭代步                                            │\n");
    printf("│  - 优点: 减少内存访问延迟                                                      │\n");
    printf("│  - 缺点: 预取距离需要调优                                                      │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ 版本5: 缓存友好优化 (Cache-Friendly Optimization)                             │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ 优化思路:                                                                    │\n");
    printf("│  - 确保所有数据 16 字节对齐 (NEON对齐要求)                                     │\n");
    printf("│  - 优化数据访问模式，利用空间局部性                                            │\n");
    printf("│  - 分离小规模和大规模FFT的处理策略                                            │\n");
    printf("│  - 减少跨缓存行访问                                                            │\n");
    printf("│  - 优点: 提高缓存命中率                                                        │\n");
    printf("│  - 缺点: 实现相对复杂                                                          │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────────────┐\n");
    printf("│ 版本6: 混合优化 (Hybrid - All Optimizations Combined)                        │\n");
    printf("├─────────────────────────────────────────────────────────────────────────────┤\n");
    printf("│ 优化思路:                                                                    │\n");
    printf("│  - 综合使用以上所有优化技术的最佳组合                                          │\n");
    printf("│  - 查找表 + NEON向量化 + 软件预取 + 缓存友好访问                              │\n");
    printf("│  - 针对不同FFT规模自适应选择最佳策略                                           │\n");
    printf("│  - 优点: 全面优化，性能最佳                                                    │\n");
    printf("│  - 缺点: 代码复杂度较高                                                        │\n");
    printf("└─────────────────────────────────────────────────────────────────────────────┘\n");
    printf("\n");
}

// ============================================================
// 主函数
// ============================================================

int main(int argc, char *argv[]) {
    print_header();
    
    // 打印优化说明
    print_optimization_notes();
    
    int test_mode = 0;
    int test_size = 4096;
    FFT_Opt_Version test_version = OPT_HYBRID;
    
    // 解析命令行参数
    if (argc > 1) {
        if (strcmp(argv[1], "--all") == 0) {
            test_mode = 1;  // 完整对比
        } else if (strcmp(argv[1], "--sweep") == 0) {
            test_mode = 2;  // 规模扫描
            if (argc > 2) {
                int v = atoi(argv[2]);
                if (v >= 0 && v <= 6) test_version = (FFT_Opt_Version)v;
            }
        } else if (strcmp(argv[1], "--size") == 0 && argc > 2) {
            test_mode = 3;  // 固定规模测试所有版本
            test_size = atoi(argv[2]);
        } else if (strcmp(argv[1], "--help") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --all           Run complete comparison of all versions\n");
            printf("  --sweep [v]     Sweep sizes for version v (0-6)\n");
            printf("  --size N        Test all versions at size N\n");
            printf("  --help          Show this help message\n");
            printf("\nVersions:\n");
            printf("  0: Original (Precompute + Partial NEON)\n");
            printf("  1: V1: Real-time sin/cos\n");
            printf("  2: V2: Lookup Table\n");
            printf("  3: V3: Enhanced NEON\n");
            printf("  4: V4: Software Prefetch\n");
            printf("  5: V5: Cache-Friendly\n");
            printf("  6: V6: Hybrid (All)\n");
            return 0;
        }
    }
    
    switch (test_mode) {
        case 0:  // 默认：测试几个关键规模的所有版本
        default:
            // 测试几个关键规模
            int key_sizes[] = {256, 1024, 4096, 16384};
            for (int i = 0; i < 4; i++) {
                test_all_versions_at_size(key_sizes[i]);
            }
            break;
            
        case 1:  // 完整对比
            test_complete_comparison();
            break;
            
        case 2:  // 规模扫描
            test_single_version_sweep(test_version);
            break;
            
        case 3:  // 固定规模测试
            test_all_versions_at_size(test_size);
            break;
    }
    
    printf("\n");
    printf("================================================================================\n");
    printf("                           测试完成 (Benchmark Complete)                         \n");
    printf("================================================================================\n");
    printf("\n");
    
    return 0;
}
