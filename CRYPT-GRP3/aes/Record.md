## 优化一 

```cpp
// old
void inv_shift_rows(uint8_t *state) {

    uint8_t i, k, s;
    uint8_t tmp[4];
    for (i = 1; i < 4; i++) {
        for(s = 0; s < i ; s++) {
            tmp[s] = state[Nb * i + Nb - i + s];
        }
        for (k = Nb-1; k > i - 1; k--) {
            state[Nb*i+k] = state[Nb*i+k-i];
        }
        for(s = 0; s < i ; s++) {
            state[Nb * i + s] = tmp[s];
        }
    }
}
```

```cpp
// new
void inv_shift_rows(uint8_t *state) {
    uint8_t tmp;

    // 第 1 行：整体循环右移 1 位
    // [4, 5, 6, 7] 变成 [7, 4, 5, 6]
    tmp = state[7];
    state[7] = state[6];
    state[6] = state[5];
    state[5] = state[4];
    state[4] = tmp;

    // 第 2 行：整体循环右移 2 位
    // [8, 9, 10, 11] 变成 [10, 11, 8, 9]
    tmp = state[8]; state[8] = state[10]; state[10] = tmp;
    tmp = state[9]; state[9] = state[11]; state[11] = tmp;

    // 第 3 行：整体循环右移 3 位
    // [12, 13, 14, 15] 变成 [13, 14, 15, 12]
    tmp = state[12];
    state[12] = state[13];
    state[13] = state[14];
    state[14] = state[15];
    state[15] = tmp;
}
```

原函数的作用是循环位移，将一个 $4 \times 4$ 矩阵（从第 $0$ 行开始）的 $1,2,3$ 行循环右移，其中第 $i$ 行右移 $i$ 位。

优化函数将所有循环展开，直接以暴力赋值。

```
aes操作结果校验通过

[规模128] aes操作耗时: 0.056000 毫秒
aes操作结果校验通过

[规模256] aes操作耗时: 0.090000 毫秒
aes操作结果校验通过

[规模512] aes操作耗时: 0.180000 毫秒
aes操作结果校验通过

[规模1024] aes操作耗时: 0.368000 毫秒
aes操作结果校验通过

[规模2048] aes操作耗时: 0.624000 毫秒
aes操作结果校验通过

[规模4096] aes操作耗时: 1.165000 毫秒
aes操作结果校验通过

[规模8192] aes操作耗时: 2.353000 毫秒
aes操作结果校验通过

[规模16384] aes操作耗时: 4.638000 毫秒
aes操作结果校验通过

[规模32768] aes操作耗时: 9.320000 毫秒
aes操作结果校验通过

[规模65536] aes操作耗时: 18.208000 毫秒
aes操作结果校验通过

[规模131072] aes操作耗时: 36.904000 毫秒
aes操作结果校验通过

[规模262144] aes操作耗时: 73.600000 毫秒
aes操作结果校验通过

[规模524288] aes操作耗时: 150.030000 毫秒
aes操作结果校验通过

[规模1048576] aes操作耗时: 298.299000 毫秒

```

```
aes操作结果校验通过

[规模128] aes操作耗时: 0.045000 毫秒
aes操作结果校验通过

[规模256] aes操作耗时: 0.114000 毫秒
aes操作结果校验通过

[规模512] aes操作耗时: 0.138000 毫秒
aes操作结果校验通过

[规模1024] aes操作耗时: 0.331000 毫秒
aes操作结果校验通过

[规模2048] aes操作耗时: 0.550000 毫秒
aes操作结果校验通过

[规模4096] aes操作耗时: 1.076000 毫秒
aes操作结果校验通过

[规模8192] aes操作耗时: 2.102000 毫秒
aes操作结果校验通过

[规模16384] aes操作耗时: 4.197000 毫秒
aes操作结果校验通过

[规模32768] aes操作耗时: 8.480000 毫秒
aes操作结果校验通过

[规模65536] aes操作耗时: 16.740000 毫秒
aes操作结果校验通过

[规模131072] aes操作耗时: 34.169000 毫秒
aes操作结果校验通过

[规模262144] aes操作耗时: 66.834000 毫秒
aes操作结果校验通过

[规模524288] aes操作耗时: 135.053000 毫秒
aes操作结果校验通过

[规模1048576] aes操作耗时: 269.063000 毫秒

```



## 优化二 

```cpp
// old
void inv_sub_bytes(uint8_t *state) {

    uint8_t i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < Nb; j++) {
            state[Nb*i+j] = inv_s_box[state[Nb*i+j]];
        }
    }
}
```

```cpp
// new1
void inv_sub_bytes(uint8_t *state) {
    for (int k = 0; k < 16; k++) {
        state[k] = inv_s_box[state[k]];
    }
}
```

```cpp
// new2
void inv_sub_bytes(uint8_t *state) {
    // 零循环开销，直接让 CPU 并发执行 16 次内存读写
    state[0]  = inv_s_box[state[0]];
    state[1]  = inv_s_box[state[1]];
    state[2]  = inv_s_box[state[2]];
    state[3]  = inv_s_box[state[3]];
    state[4]  = inv_s_box[state[4]];
    state[5]  = inv_s_box[state[5]];
    state[6]  = inv_s_box[state[6]];
    state[7]  = inv_s_box[state[7]];
    state[8]  = inv_s_box[state[8]];
    state[9]  = inv_s_box[state[9]];
    state[10] = inv_s_box[state[10]];
    state[11] = inv_s_box[state[11]];
    state[12] = inv_s_box[state[12]];
    state[13] = inv_s_box[state[13]];
    state[14] = inv_s_box[state[14]];
    state[15] = inv_s_box[state[15]];
}
```

这是一个查表并赋值的函数

依旧是循环展开，new1 是将双层循环展开成一层，new2 是将循环完全展开，可以看到明显提升



old 结果见上（每一个优化的 old 都是上一个优化的 new）

```
new1结果

aes操作结果校验通过

[规模128] aes操作耗时: 0.047000 毫秒
aes操作结果校验通过

[规模256] aes操作耗时: 0.068000 毫秒
aes操作结果校验通过

[规模512] aes操作耗时: 0.134000 毫秒
aes操作结果校验通过

[规模1024] aes操作耗时: 0.262000 毫秒
aes操作结果校验通过

[规模2048] aes操作耗时: 0.527000 毫秒
aes操作结果校验通过

[规模4096] aes操作耗时: 1.082000 毫秒
aes操作结果校验通过

[规模8192] aes操作耗时: 2.176000 毫秒
aes操作结果校验通过

[规模16384] aes操作耗时: 4.129000 毫秒
aes操作结果校验通过

[规模32768] aes操作耗时: 7.938000 毫秒
aes操作结果校验通过

[规模65536] aes操作耗时: 16.092000 毫秒
aes操作结果校验通过

[规模131072] aes操作耗时: 32.940000 毫秒
aes操作结果校验通过

[规模262144] aes操作耗时: 66.424000 毫秒
aes操作结果校验通过

[规模524288] aes操作耗时: 131.932000 毫秒
aes操作结果校验通过

[规模1048576] aes操作耗时: 265.291000 毫秒

```

```
new2 结果

aes操作结果校验通过

[规模128] aes操作耗时: 0.043000 毫秒
aes操作结果校验通过

[规模256] aes操作耗时: 0.072000 毫秒
aes操作结果校验通过

[规模512] aes操作耗时: 0.129000 毫秒
aes操作结果校验通过

[规模1024] aes操作耗时: 0.259000 毫秒
aes操作结果校验通过

[规模2048] aes操作耗时: 0.520000 毫秒
aes操作结果校验通过

[规模4096] aes操作耗时: 1.020000 毫秒
aes操作结果校验通过

[规模8192] aes操作耗时: 2.033000 毫秒
aes操作结果校验通过

[规模16384] aes操作耗时: 3.988000 毫秒
aes操作结果校验通过

[规模32768] aes操作耗时: 7.725000 毫秒
aes操作结果校验通过

[规模65536] aes操作耗时: 15.457000 毫秒
aes操作结果校验通过

[规模131072] aes操作耗时: 31.491000 毫秒
aes操作结果校验通过

[规模262144] aes操作耗时: 62.447000 毫秒
aes操作结果校验通过

[规模524288] aes操作耗时: 125.339000 毫秒
aes操作结果校验通过

[规模1048576] aes操作耗时: 249.496000 毫秒

```



## 优化三

```cpp
// old
void add_round_key(uint8_t *state, uint8_t *w, uint8_t r) {

    uint8_t c;

    for (c = 0; c < Nb; c++) {
        state[Nb*0+c] = state[Nb*0+c]^w[4*Nb*r+4*c+0];   //debug, so it works for Nb !=4
        state[Nb*1+c] = state[Nb*1+c]^w[4*Nb*r+4*c+1];
        state[Nb*2+c] = state[Nb*2+c]^w[4*Nb*r+4*c+2];
        state[Nb*3+c] = state[Nb*3+c]^w[4*Nb*r+4*c+3];
    }
}
```

```cpp
// new
void add_round_key(uint8_t *state, uint8_t *w, uint8_t r) {
    // 提前计算出当前轮密钥的起始指针，消灭内层的 4*Nb*r 乘法
    uint8_t *round_key = w + (16 * r);

    // 完全循环展开，硬编码下标
    state[0]  ^= round_key[0];
    state[4]  ^= round_key[1];
    state[8]  ^= round_key[2];
    state[12] ^= round_key[3];

    state[1]  ^= round_key[4];
    state[5]  ^= round_key[5];
    state[9]  ^= round_key[6];
    state[13] ^= round_key[7];

    state[2]  ^= round_key[8];
    state[6]  ^= round_key[9];
    state[10] ^= round_key[10];
    state[14] ^= round_key[11];

    state[3]  ^= round_key[12];
    state[7]  ^= round_key[13];
    state[11] ^= round_key[14];
    state[15] ^= round_key[15];
}
```

这个函数是用轮密钥（round_key）加密，就是将一个 $4 \times 4$ 的矩阵（加密矩阵）与另一个 $4 \times 4$ 矩阵（密钥矩阵）进行异或，异或的模式类似矩阵乘法的模式，加密矩阵的第 $i$ 列与密钥矩阵的第 $i$ 行异或，得出一个新的 $4 \times 4$ 的矩阵。



```
new 结果

aes操作结果校验通过

[规模128] aes操作耗时: 0.039000 毫秒
aes操作结果校验通过

[规模256] aes操作耗时: 0.058000 毫秒
aes操作结果校验通过

[规模512] aes操作耗时: 0.112000 毫秒
aes操作结果校验通过

[规模1024] aes操作耗时: 0.225000 毫秒
aes操作结果校验通过

[规模2048] aes操作耗时: 0.456000 毫秒
aes操作结果校验通过

[规模4096] aes操作耗时: 0.880000 毫秒
aes操作结果校验通过

[规模8192] aes操作耗时: 1.710000 毫秒
aes操作结果校验通过

[规模16384] aes操作耗时: 3.437000 毫秒
aes操作结果校验通过

[规模32768] aes操作耗时: 6.990000 毫秒
aes操作结果校验通过

[规模65536] aes操作耗时: 13.427000 毫秒
aes操作结果校验通过

[规模131072] aes操作耗时: 27.475000 毫秒
aes操作结果校验通过

[规模262144] aes操作耗时: 55.598000 毫秒
aes操作结果校验通过

[规模524288] aes操作耗时: 111.945000 毫秒
aes操作结果校验通过

[规模1048576] aes操作耗时: 222.601000 毫秒

```



## 优化四

```cpp
// old
void inv_mix_columns(uint8_t *state) {

    uint8_t a[] = {0x0e, 0x09, 0x0d, 0x0b}; // a(x) = {0e} + {09}x + {0d}x2 + {0b}x3
    uint8_t i, j, col[4], res[4];

    for (j = 0; j < Nb; j++) {
        for (i = 0; i < 4; i++) {
            col[i] = state[Nb*i+j];
        }

        coef_mult(a, col, res);

        for (i = 0; i < 4; i++) {
            state[Nb*i+j] = res[i];
        }
    }
}
```

```cpp
// new
void inv_mix_columns(uint8_t *state) {
    uint8_t s0, s1, s2, s3;

    for (int j = 0; j < 4; j++) {
        // 读取第 j 列的所有数
        s0 = state[j];
        s1 = state[j + 4];
        s2 = state[j + 8];
        s3 = state[j + 12];

        // 
        state[j]      = gmult(0x0e, s0) ^ gmult(0x0b, s1) ^ gmult(0x0d, s2) ^ gmult(0x09, s3);
        state[j + 4]  = gmult(0x09, s0) ^ gmult(0x0e, s1) ^ gmult(0x0b, s2) ^ gmult(0x0d, s3);
        state[j + 8]  = gmult(0x0d, s0) ^ gmult(0x09, s1) ^ gmult(0x0e, s2) ^ gmult(0x0b, s3);
        state[j + 12] = gmult(0x0b, s0) ^ gmult(0x0d, s1) ^ gmult(0x09, s2) ^ gmult(0x0e, s3);
    }
}
```

原代码作用是进行进一步的加密（混淆），将一个 $4 \times 4$ 的矩阵的每一列单独提取出来，对于一列数，我们将其与一个 $4 \times 4$ 的矩阵做“矩阵乘法”；

- 首先这个 $4 \times 4$ 的矩阵是由原函数中的 ` uint8_t a[] = {0x0e, 0x09, 0x0d, 0x0b}; // a(x) = {0e} + {09}x + {0d}x2 + {0b}x3` 

  这段代码来定义的，根据 a 数组构造一个形如

  | a[0] | a[3] | a[2] | a[1] |
  | ---- | ---- | ---- | ---- |
  | a[1] | a[0] | a[3] | a[2] |
  | a[2] | a[1] | a[0] | a[3] |
  | a[3] | a[2] | a[1] | a[0] |

  的一个 $4 \times 4$ 的矩阵

- “矩阵乘法”不是原本意义上的乘法，原本意义上的矩阵乘法是“mult - add”矩阵乘法，这里的是 “gmult - xor” 矩阵乘法，其中 gmult 是查表（也是一个二元运算），表的具体定义在 `gmult.cpp` 中，xor 是指异或；也就是说将原本矩阵乘法中本应该相乘的地方用 gmult 查表来代替，原本加法的地方用 xor 来代替。

优化函数依旧是将循环展开；同时将原函数中的 `coef_mult` 函数写进该函数中，舍去了 `coef_mult` 中读数组的过程，将 a 数组用常数代替；并舍去了中间变量最后赋值回去的过程，直接将做得值存储在原数组（加密矩阵）上



```
new 结果

aes操作结果校验通过

[规模128] aes操作耗时: 0.034000 毫秒
aes操作结果校验通过

[规模256] aes操作耗时: 0.048000 毫秒
aes操作结果校验通过

[规模512] aes操作耗时: 0.092000 毫秒
aes操作结果校验通过

[规模1024] aes操作耗时: 0.182000 毫秒
aes操作结果校验通过

[规模2048] aes操作耗时: 0.357000 毫秒
aes操作结果校验通过

[规模4096] aes操作耗时: 0.685000 毫秒
aes操作结果校验通过

[规模8192] aes操作耗时: 1.442000 毫秒
aes操作结果校验通过

[规模16384] aes操作耗时: 2.933000 毫秒
aes操作结果校验通过

[规模32768] aes操作耗时: 5.537000 毫秒
aes操作结果校验通过

[规模65536] aes操作耗时: 10.999000 毫秒
aes操作结果校验通过

[规模131072] aes操作耗时: 22.744000 毫秒
aes操作结果校验通过

[规模262144] aes操作耗时: 46.606000 毫秒
aes操作结果校验通过

[规模524288] aes操作耗时: 107.359000 毫秒
aes操作结果校验通过

[规模1048576] aes操作耗时: 182.428000 毫秒

```

