## TDA4-A72-Linux

#### 项目路径

飞书文档	组内共享->Task代码及测试结果->加解密算法->TDA4-A72-Linux.zip

#### 配置&编译&运行

$项目路径/param.h：配置优化策略是否启用

编译: 正常cmake项目编译 
mkdir build
cmake ..
make
环境要求：需安装aarch64-linux-gnu-gcc交叉编译器

运行：
编译好后会生成：aes_encrypt md5_hash sha256_hash三个文件
scp拷到tda4上之后直接运行即可

##### 代码说明
、
**各Task接口**：$项目目录/[task_name]/[task_name]_interface.h