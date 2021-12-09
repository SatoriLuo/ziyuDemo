#include "tcpDemo.h"

int main()
{
    tcpDemo demo;
    if(!demo.InitAll() || !demo.createShareMem())
    {
        std::cout << "demo fail,please restart!!!" << std::endl;
        demo.MyPause();
        return -1;
    }
    demo.connectSlave();
    std::cout << "start demo." << std::endl;
    demo.MyPause();
    std::thread MonitorDataThread(&tcpDemo::MonitorTerminal,&demo);
    demo.MonitorInputShareMem();
    if(MonitorDataThread.joinable())
        MonitorDataThread.join();
    return 0;
}