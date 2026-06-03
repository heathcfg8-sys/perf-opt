gcc -O3 -I./ -I.. -I./md5_optimized -I./md5_test -c md5_test/md5_linux.c -o md5_linux.o
gcc -O3 -I./ -I.. -I./md5_optimized -I./md5_test -c md5_test/md5_linux_test.c -o md5_linux_test.o
gcc -O3 -I./ -I.. -I./md5_optimized -I./md5_test -c md5_test/timestamp.c -o timestamp.o
g++ -O3 -I./ -I.. -I./md5_optimized -I./md5_test -c md5_optimized/md5.cpp -o md5.o
g++ -O3 -I./ -I.. -I./md5_optimized -I./md5_test -c md5_optimized/md5_linux_adapter.cpp -o md5_linux_adapter.o
g++ md5_linux.o md5_linux_test.o timestamp.o md5.o md5_linux_adapter.o -o md5_test_optimized
./md5_test_optimized