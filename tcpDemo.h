#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <thread>
#include <ctime>
#include <fstream>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#include <Winsock2.h>
#include <winsock.h>
#include <Windows.h>
#include <tchar.h>
#include <windows.h>
//#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ctype.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#endif

#ifdef TESTDEMO
#define IP_ADDR "192.168.10.121"
#else
#define IP_ADDR "192.168.10.87"
#endif
#define IP_PORT_COMMND 1503
#define IP_PORT_MONITOR  1504
#ifdef _WIN32
#define CONFIG_FILE_PATH         ".\\config.ini"
#define SHAREMEM_FILEPATH_INPUT  ".\\shareInput.txt"
#define SHAREMEM_FILEPATH_OUTPUT ".\\shareOutput.txt"
#define SHAREMEM_FILENAME_INPUT  "shareMemInput"
#define SHAREMEM_FILENAME_OUTPUT "shareMemOutput"
#define MONITOR_TUPPU_FILE_PATH  ".\\monitorTuopu.txt"  
#else
#define CONFIG_FILE_PATH         "./config.ini"
#define SHAREMEM_FILEPATH_INPUT  "./shareInput.txt"
#define SHAREMEM_FILEPATH_OUTPUT "./shareOutput.txt"
#define MONITOR_TUPPU_FILE_PATH  "./monitorTuopu.txt"  
#endif
#define SHAREMEM_FILE_SIZE  2 * 1024
#define PER_COMMND_SIZE     12
#define TCP_LISTEN_QUEUE    20
#define SHAREMEM_OUTPUT_FLAG   2000
#define SHAREMEM_INPUT_FLAG    2047
#define OUTPUT_DATA_LENGTH  1999
#define OUTPUT_DATA_PER_LENGTH 120
#define OUTPUT_DATA_NUMBER  1000
#define REQUEST_COMMND_REQUESTSIZE_POSITION    5
#define RESPONSE_DATA_BEGIN_POSITION           10
#define MONITOR_TUPPU_POSITION                 32 * 2 + 1

#define INIT_BEGIN_POSITION                    16
#define INIT_BEGIN_LENGTH                      16
#define SOCKET_RECV_BUF_SIZE                   1024
#define INTI_BEGIN_POSITION_YAOCE_NODE_NUMBER  1
#define INIT_BEGIN_POSITION_NODE_NUMBER        3
#define ALL_DATA_BEGIN_POSITION                40
#define MONITOR_TUPPU_CHANGE_POSITION          40
#define TERMINAL_DATA_SIMULINK_LENGTH          8

#define CONFIG_LINE_INDEX_IPADDR               1
#define CONFIG_LINE_INDEX_COMMND_PORT          2
#define CONFIG_LINE_INDEX_MONITOR_PORT         3

#define TUOPO_FLAG_SIZE                        2

enum socket_status
{
        socket_status_disconnect = 0,
        socket_status_connected,
        socket_status_connecting
};

class tcpDemo
{
public:
        tcpDemo();
        ~tcpDemo();

        
private:
        void MySleep(int ims);
        bool sendCommndAndRecv(int socketfd,std::string strCommnd,char* recvBuf,int recvBufLength);
        long long getTimeInMS();
        bool getConfigFromFile(std::string strFilePath);
        bool IsTuopuChanged(char* oldStatus,char* newStatus,int iSize);
        bool IsInitDataValid(char* data);
public:
        bool m_bConnectSuc;
        bool bInitStatus;
        bool InitAll();
        bool Init();
        void MyPause();
        bool ReconnectSlave();
        bool connectSlave();
        bool createInputShareMem();
        bool createOutputShareMem();
        bool disconnectSlave();
        bool getDataFromSlave();
        void MonitorInputShareMem();
        void MonitorTerminal();
        void MonitorTuoPu();
        bool createShareMem();
        void setRequestCommnd(std::string &strCommnd,int iBeginPos,int iLength);
        void getAllData(bool bIgnoreFirst = false);
#ifdef _WIN32
        bool initialization();
        HANDLE m_shareMemInputHandleWindows;
        HANDLE m_shareMemOutputHandleWindows;
        HANDLE m_shareMemInputCreateFileWindows;
        HANDLE m_shareMemOutputCreateFileWindows;
#endif
private:
        int m_socketfd;
        int m_socketlistenfd;
        int m_shareMemOutputHandle;
        int m_shareMemInputHandle;
        char* m_pShareMemInput;
        char* m_pShareMemOutput;
        int m_iWriteDataPoll;
        std::recursive_mutex m_TuopoMutex;
        bool m_bTuopoStatusChanged;
        char m_cTuopoTerminalStatus;

        unsigned int m_ui_NodeYaocePointsNumber;//节点遥测点数量
        unsigned int m_ui_NodesNumber;//节点数量
        socket_status m_E_CommndsocketStatus;
        socket_status m_E_MonitorSocketStatus;

        std::string m_strIPAddr;
        int m_iCommndPort;
        int m_iMonitorPort;

        char m_TuopoFlag[TUOPO_FLAG_SIZE];
        bool m_bInitTuopoFlag;
};