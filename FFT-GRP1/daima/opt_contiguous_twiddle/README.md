# fft_acore_linux

共有如下四个代码，call_test.c、main.c、fft_a.c、fft_a.h.
call_test.c用于测试fft_linux_iopointer()的外部调用
main.c用于测试fft_linux_ioself_profiling(),打印不同规模的运行时间和结果验证
fft_a.h fft_a.c包含计算和基础功能的实现


## 输入数据格式
1、fft_linux_iopointer(int size, float *input_real, float *input_imag)
五个参数中size为点数(问题规模)，其余float *类型参数，代表输入的实部以及虚部数据，计算后输出数据将覆盖输入数据，若要验证结果，需提前另外保存输入。

初始化输入：实部为随机值（-32768~32767），虚部为0（模拟PCM）

2、fft_linux_ioself_profiling(int size)和fft_linux_ioself(int size)只有size为点数作为参数输入，输入由随机数代替。

## 关于验证

验证思路为将fft计算后的结果数据再进行ifft计算，并与输入数据的实部数据进行对比。

若打印出max_error为较小的小数时 基本就可认为测试通过，只存在细小的精度问题。

## 编译 运行

1、测试运行fft_linux_ioself_profiling()；
主机编译 aarch64-linux-gnu-gcc -static -O2 -o fft main.c fft_a.c -march=armv8-a -lm
开发板运行 ./fft

2、测试运行fft_linux_iopointer();
aarch64-linux-gnu-gcc -static -O2 -o fft call_test.c fft_a.c -march=armv8-a -lm
./fft

