编译说明：
编译器用的是mingw
Windows环境测试编译命令:g++ -g  -Wall -std=c++11 -o tcpDemo .\tcpDemo.h .\tcpDemo.cpp .\main.cpp -lwsock32

linux编译命令  g++ -g -Wall -std=c++11 -o tcpDemo tcpDemo.h tcpDemo.cpp main.cpp -lpthread
linux版本未测试！！！

现在的版本（20210802）:
1、程序启动时会去可执行文件相同目录下读取配置文件的信息(config.ini)
    配置文件的内容（行）：1、ip地址，2、用于写命令的端口号，3、用于检测仿真状态的端口号
2、通过文件映射建立两个共享内存，文件要有权限打开，文件路径未可执行程序的相同路径，
     shareInput.txt用于java端写入命令，shareOutput.txt用于C++端检测仿真的变化，有变化的情况下通过该文件通知java
     文件的大小为2048个字节，两个共享内存的同步机制都为一个标记位去限制读写。