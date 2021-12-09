#include "tcpDemo.h"

tcpDemo::tcpDemo()
{
    m_socketfd = -1;
    m_socketlistenfd = -1;
    m_shareMemOutputHandle = -1;
    m_shareMemInputHandle = -1;
    m_pShareMemInput = NULL;
    m_pShareMemOutput = NULL;
    bInitStatus = false;
    m_socketlistenfd = -1;
    m_bTuopoStatusChanged = false;
    m_ui_NodeYaocePointsNumber = -1;
    m_ui_NodesNumber = -1;
    m_cTuopoTerminalStatus = 0x00;
    m_E_CommndsocketStatus = socket_status_disconnect;
    m_E_MonitorSocketStatus = socket_status_disconnect;
    m_strIPAddr = IP_ADDR;
    m_iCommndPort = IP_PORT_COMMND;
    m_iMonitorPort = IP_PORT_MONITOR;
    for(size_t i = 0;i < TUOPO_FLAG_SIZE;i++)
        m_TuopoFlag[i] = 0x00;
    m_bInitTuopoFlag = true;
}

tcpDemo::~tcpDemo()
{
    disconnectSlave();
#ifdef _WIN32
    WSACleanup();
    // 解除文件映射，关闭内存映射文件对象句柄
	::UnmapViewOfFile(m_pShareMemInput);
    ::UnmapViewOfFile(m_pShareMemOutput);
	::CloseHandle(m_shareMemInputHandleWindows);
    ::CloseHandle(m_shareMemOutputHandleWindows);
    ::CloseHandle(m_shareMemInputCreateFileWindows);
    ::CloseHandle(m_shareMemOutputCreateFileWindows);
#else
    bool bDetachInput = false,bDetachOutput = false;
    if(shmdt(m_pShareMemInput) == -1)
    {
        std::cout << "shmdt share mem input fail." << std::endl;
        bDetachInput = false;
    }
    else
    {
        std::cout << "shmdt share mem input success." << std::endl;
        bDetachInput = true;
    }
    
    if(shmdt(m_pShareMemOutput) == -1)
    {
        std::cout << "shmdt share mem output fail" << std::endl;
        bDetachOutput = false;
    }
    else
    {
        std::cout << "shmdt share mem output success" << std::endl;
        bDetachOutput = true;
    }

    if(bDetachInput)
    {
        if(shmctl(m_shareMemInputHandle, IPC_RMID, 0) == -1)
            std::cout << "delete share mem input fail." << std::endl;
        else
            std::cout << "delete share mem input success." << std::endl;
    }

    if(bDetachOutput)
    {
        if(shmctl(m_shareMemOutputHandle, IPC_RMID, 0) == -1)
            std::cout << "delete share mem input fail." << std::endl;
        else
            std::cout << "delete share mem input success." << std::endl;
    }

#endif
}

void tcpDemo::MySleep(int ims)
{
#ifdef _WIN32
        Sleep(ims);
#else
        sleep(ims);
#endif
}

void tcpDemo::MyPause()
{
#ifdef _WIN32
        system("pause");
#else
        std::cout << "please input word to continue:";
        char temp = getchar();
#endif
}

long long tcpDemo::getTimeInMS()
{
    long long result = 0;
#ifdef _WIN32
    result = GetTickCount();
#else 
    struct timeval tv;
    gettimeofday(&tv,NULL);
    result = tv.tv_sec*1000 + tv.tv_usec/1000;
#endif
    return result;
}

bool tcpDemo::getConfigFromFile(std::string strFilePath)
{
    if(strFilePath.empty())
    {
        std::cout << "get config from file fail,file path empty";
        return false;
    }
    std::ifstream ifConfig;
    ifConfig.open(strFilePath,std::ios::in);
    if(!ifConfig.is_open())
    {
        std::cout << "get config from file fail,open file fail.";
        return false;
    }
    char buf[1024] = {0};
    int iLine = 1;
    while(ifConfig.getline(buf,sizeof(buf)))
    {
        std::stringstream tempStream(buf);
        if(iLine++ == CONFIG_LINE_INDEX_IPADDR)
            tempStream >> m_strIPAddr;
        else if(iLine++ == CONFIG_LINE_INDEX_COMMND_PORT)
            tempStream >> m_iCommndPort;
        else if(iLine++ == CONFIG_LINE_INDEX_MONITOR_PORT)
            tempStream >> m_iMonitorPort;
        else
            continue;
    }
    std::cout << "ip = " << m_strIPAddr << ",commnd port = " << m_iCommndPort << ",monitor port = " << m_iMonitorPort << std::endl;
    return true;
}

bool tcpDemo::InitAll()
{
    getConfigFromFile(CONFIG_FILE_PATH);
#ifdef _WIN32
	initialization();
#else

#endif
    return true;
}

bool tcpDemo::Init()
{
    if(m_E_MonitorSocketStatus != socket_status_connected)
    {
        std::cout << "init not ready,please restart." << std::endl;
        ReconnectSlave();
        return false;
    }
    std::string strCommnd(12,0);
    setRequestCommnd(strCommnd,INIT_BEGIN_POSITION,INIT_BEGIN_LENGTH);
    char* recvBuf = new char[SOCKET_RECV_BUF_SIZE];
    memset(recvBuf,'\0',SOCKET_RECV_BUF_SIZE);
    if(!sendCommndAndRecv(m_socketlistenfd,strCommnd,recvBuf,SOCKET_RECV_BUF_SIZE))
    {
        std::cout << "init fail,send data or recv data fail" << std::endl;
        bInitStatus = false;
        m_E_MonitorSocketStatus = socket_status_disconnect;
        return false;
    }
    if(!IsInitDataValid(recvBuf + 9 + 6))
        return false;
    m_ui_NodeYaocePointsNumber = (unsigned int)((int)(*(recvBuf + 9 + 3)) + 256 * ((int)*(recvBuf + 9 + 2)));
    m_ui_NodesNumber = (unsigned int)((int)(*(recvBuf + 9 + 5)) + 256 * ((int)*(recvBuf + 9 + 4)));
    (*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) = (int)1;
    memcpy(m_pShareMemOutput + INIT_BEGIN_POSITION * 2,recvBuf + 9,INIT_BEGIN_LENGTH * 2);
    (*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) = (int)0;
    std::cout << std::dec << "Init success,Yaoce Node Number = " << m_ui_NodeYaocePointsNumber << ",Node number = " << m_ui_NodesNumber << std::endl;
    bInitStatus = true;
    return true;
}

bool tcpDemo::IsInitDataValid(char* data)
{
    return (*data == 0x55) && ((unsigned char)*(data + 1) == 0xAA);
}

bool tcpDemo::sendCommndAndRecv(int socketfd,std::string strCommnd,char* recvBuf,int recvBufLength)
{
    if(strCommnd.empty() || recvBuf == NULL || recvBufLength <= 0 || socketfd == -1)
    {
        std::cout << "send Commnd and recv fail,param error." << std::endl;
        return false;
    }
#ifdef _WIN32
    int send_len = send(socketfd, strCommnd.c_str(), strCommnd.size(), 0);
#else
    int send_len = send(socketfd, strCommnd.c_str(), strCommnd.size() , 0);//向TCP连接的另一端发送数据。
#endif
    if(send_len < 0)
    {
        std::cout << "send Commnd and Recv send data fail." << std::endl;
        ReconnectSlave();
        return false;
    }
#ifdef _WIN32
    int recv_len = recv(socketfd,recvBuf,recvBufLength,0);    
#else
    int recv_len = recv(socketfd, recvBuf, recvBufLength, 0);//从TCP连接的另一端接收数据。
#endif
    if(recv_len < 0)
    {
        ReconnectSlave();
        std::cout << "recv data fail." << std::endl;
        return false;
    }
    return true;
}

bool tcpDemo::ReconnectSlave()
{
    m_E_CommndsocketStatus = socket_status_connecting;
    MySleep(1000 * 2);
    std::cout << "try to reconnect..." << std::endl;
    connectSlave();
    return true;
}

bool tcpDemo::connectSlave()
{
#ifdef _WIN32
    if(m_E_CommndsocketStatus != socket_status_connected)
    {
	    SOCKADDR_IN server_addr;
	    server_addr.sin_family = AF_INET;
	    server_addr.sin_addr.S_un.S_addr = inet_addr(m_strIPAddr.c_str());
	    server_addr.sin_port = htons(m_iCommndPort);
	    m_socketfd = (int)socket(AF_INET, SOCK_STREAM, 0);
        std::cout << "m_socketfd = " << m_socketfd;
	    if (connect(m_socketfd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		    std::cout << ",connect slave fail,m_socketfd connect fail." << std::endl;
            m_socketfd = -1;
	    }
        else
        {
            std::cout << ",connect commnd port success." << std::endl;
            m_E_CommndsocketStatus = socket_status_connected;
        }
    }

    if(m_E_MonitorSocketStatus != socket_status_connected)
    {
        SOCKADDR_IN listen_addr;
	    listen_addr.sin_family = AF_INET;
	    listen_addr.sin_addr.S_un.S_addr = inet_addr(m_strIPAddr.c_str());
	    listen_addr.sin_port = htons(m_iMonitorPort);
        m_socketlistenfd = (int)socket(AF_INET, SOCK_STREAM, 0);
        std::cout << "m_socketlistenfd = " << m_socketlistenfd;
	    if (connect(m_socketlistenfd, (sockaddr*)&listen_addr, sizeof(listen_addr)) == SOCKET_ERROR) {
		    std::cout << ",connect slave fail,m_socketlistenfd connect fail." << std::endl;
            m_socketlistenfd = -1;
	    }
        else
        {
            std::cout << ",connect monitor port success." << std::endl;
            m_E_MonitorSocketStatus = socket_status_connected;
            while(1)
            {
                if(Init())
                    break;
            }
        }
    }
#else
    ///定义sockfd
    if(m_E_CommndsocketStatus != socket_status_connected)
    {
        m_socketfd = socket(AF_INET,SOCK_STREAM, 0);
        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(m_iCommndPort);  //服务器端口
        servaddr.sin_addr.s_addr = inet_addr(m_strIPAddr.c_str());  //服务器ip，inet_addr用于IPv4的IP转换（十进制转换为二进制）
        if (connect(m_socketfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            std::cout << "connect slave fail,m_socketfd connect fail.\n" << std::endl;
            m_socketfd = -1;
        }
        else
        {
            std::cout << ",connect commnd port success." << std::endl;
            m_E_CommndsocketStatus = socket_status_connected;
        }
    }
    
    if(m_E_MonitorSocketStatus != socket_status_connected)
    {
        m_socketlistenfd = socket(AF_INET,SOCK_STREAM, 0);
        struct sockaddr_in listen_addr;
        memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(m_iMonitorPort);  //服务器端口
        listen_addr.sin_addr.s_addr = inet_addr(m_strIPAddr.c_str());  //服务器ip，inet_addr用于IPv4的IP转换（十进制转换为二进制）
        if (connect(m_socketlistenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
        {
            std::cout << "connect slave fail,m_socketlistenfd connect fail.\n" << std::endl;
            m_socketlistenfd = -1;
        }
        else
        {
            std::cout << ",connect monitor port success." << std::endl;
            m_E_MonitorSocketStatus = socket_status_connected;
            while(!Init());
        }
    }
#endif
    std::cout << "commnd socke status: " << (int)m_E_CommndsocketStatus << ",monitor socket status: " << m_E_MonitorSocketStatus << std::endl;
    return true;
}
                           

bool tcpDemo::disconnectSlave()
{
    if(m_socketfd != -1)
    {
#ifdef _WIN32
        closesocket(m_socketfd);
#else
        close(m_socketfd);
#endif
    }
    m_E_CommndsocketStatus = socket_status_disconnect;
    return true;
}

void tcpDemo::getAllData(bool bIgnoreFirst)
{
    int totalLength = (TERMINAL_DATA_SIMULINK_LENGTH + m_ui_NodeYaocePointsNumber * m_ui_NodesNumber);
    int iRequestTime = (int)(totalLength / (OUTPUT_DATA_PER_LENGTH));
    long long lltime = getTimeInMS();
    std::cout << "begin to get all data,time = " << lltime << ",request time =" << iRequestTime << std::endl;
    char* buffer = new char[SOCKET_RECV_BUF_SIZE];
    int iRequestBeginCal = 0;
    for(int i = 0;i <= iRequestTime;i++)
    {
        if(bIgnoreFirst && i ==0)
            continue;
        std::cout << "i = " << i << ",begin." << std::endl;
        int iEachLength = totalLength - iRequestBeginCal > OUTPUT_DATA_PER_LENGTH ? 
                            OUTPUT_DATA_PER_LENGTH : totalLength - iRequestBeginCal;
        std::string strCommnd(12,0);
        setRequestCommnd(strCommnd,iRequestBeginCal + ALL_DATA_BEGIN_POSITION,iEachLength);
        std::cout << "commnd: commnd begin position = " << iRequestBeginCal + ALL_DATA_BEGIN_POSITION 
                    << ",length = " << iEachLength << std::endl;
        memset(buffer, 0 ,SOCKET_RECV_BUF_SIZE);
        if(!sendCommndAndRecv(m_socketlistenfd,strCommnd,buffer,SOCKET_RECV_BUF_SIZE))
        {
            m_E_MonitorSocketStatus = socket_status_disconnect;
            iRequestBeginCal += iEachLength;
            continue;
        }
        std::cout << "copy data to share mem : begin position = " << std::dec << (iRequestBeginCal + ALL_DATA_BEGIN_POSITION) * 2 
                    << ",length = " << std::dec << iEachLength * 2 << std::endl;
        std::cout << "share mem output flag data = " << std::dec << (int)(*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) << std::endl;
        if((int)(*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) == 1)
            memcpy(m_pShareMemOutput + ((iRequestBeginCal + ALL_DATA_BEGIN_POSITION) * 2),buffer + 9,iEachLength * 2);
        std::cout << "i = " << i << ",end." << std::endl;
        iRequestBeginCal += iEachLength;
    }
    std::cout << "share mem output flag data = " << std::dec << (int)(*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) << std::endl;
    if((int)(*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) == (int)1)
        (*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) = (int)0;
    lltime = getTimeInMS();
    std::cout << "end get all data,time = " << lltime << std::endl;
}

bool tcpDemo::createInputShareMem()
{
#ifdef _WIN32
    // 首先试图打开一个命名的内存映射文件对象  
	m_shareMemInputHandleWindows = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, SHAREMEM_FILENAME_INPUT);
    //system("pause");
	if (NULL == m_shareMemInputHandleWindows)
	{
        std::cout << "try to open input share mem fail.\n" << std::endl;
        // 打开失败，创建之
        m_shareMemInputCreateFileWindows = ::CreateFile(SHAREMEM_FILEPATH_INPUT, GENERIC_READ | GENERIC_WRITE,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
        if(m_shareMemInputCreateFileWindows == NULL)
        {
            std::cout << "create file fail,reason : " << GetLastError() << std::endl;
            MyPause();
            return false;
        }
	    m_shareMemInputHandleWindows = ::CreateFileMapping(m_shareMemInputCreateFileWindows,
		                            NULL, PAGE_READWRITE, 0, SHAREMEM_FILE_SIZE, SHAREMEM_FILENAME_INPUT);
	    // 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
        if(m_shareMemInputHandleWindows == NULL)
        {
            std::cout << "create file mapping fail,reason :" << std::hex << GetLastError() << std::endl;
            MyPause();
            return false;
        }
        std::cout << "m_shareMemInputHandleWindows = " << m_shareMemInputHandleWindows << std::endl;
		m_pShareMemInput = (char*)::MapViewOfFile(m_shareMemInputHandleWindows, FILE_MAP_ALL_ACCESS, 0, 0, 0);  
        if(m_pShareMemInput == NULL)
        {
            std::cout << "MapViewOfFile fail,reason = " << std::hex << GetLastError() << std::endl;
            MyPause();
            return false;
        }
        std::cout << "m_pShareMemInput = " << &m_pShareMemInput << std::endl;
        std::cout << "create input share mem success." << std::endl;
    }
#else
    key_t key = ftok(SHAREMEM_FILEPATH_INPUT,1);
    if(key == -1)
    {
        std::cout << "createInputShareMem fail,ftok fail." << std::endl;
        return false;
    }
    std::cout << "createInputShareMem ,ftok success." << std::endl;
    m_shareMemInputHandle = shmget(key, SHAREMEM_FILE_SIZE, IPC_CREAT | 0666);
    if(m_shareMemInputHandle == -1)
    {
        std::cout << "createInputShareMem fail,shmget fail." << std::endl;
        return false;
    }
    std::cout << "createInputShareMem ,shmget success." << std::endl;
    m_pShareMemInput = (char*)shmat(m_shareMemInputHandle,(void*)0,0);
    std::cout << "createInputShareMem ,shmat success." << std::endl;
#endif
    memset(m_pShareMemInput,'\0',SHAREMEM_FILE_SIZE);
    return true;
}

bool tcpDemo::createOutputShareMem()
{
#ifdef _WIN32
    m_shareMemOutputHandleWindows = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, SHAREMEM_FILENAME_OUTPUT);
    //system("pause");
	if (NULL == m_shareMemOutputHandleWindows)
	{
        std::cout << "try to open output share mem fail.\n" << std::endl;
        // 打开失败，创建之
        m_shareMemOutputCreateFileWindows = ::CreateFile(SHAREMEM_FILEPATH_OUTPUT, GENERIC_READ | GENERIC_WRITE,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
        if(m_shareMemOutputCreateFileWindows == NULL)
        {
            std::cout << "create file fail,reason : " << GetLastError() << std::endl;
            MyPause();
            return false;
        }
	    m_shareMemOutputHandleWindows = ::CreateFileMapping(m_shareMemOutputCreateFileWindows,
		                            NULL, PAGE_READWRITE, 0, SHAREMEM_FILE_SIZE, SHAREMEM_FILENAME_OUTPUT);
        if(m_shareMemOutputHandleWindows == NULL)
        {
            std::cout << "create file mapping fail,reason :" << std::hex << GetLastError() << std::endl;
            MyPause();
            return false;
        }
        std::cout << "m_shareMemOutputHandleWindows = " << m_shareMemOutputHandleWindows << std::endl;
	    // 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
		m_pShareMemOutput = (char*)::MapViewOfFile(m_shareMemOutputHandleWindows, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if(m_pShareMemOutput == NULL)
        {
            std::cout << "MapViewOfFile fail,reason = " << std::hex << GetLastError() << std::endl;
            MyPause();
            return false;
        } 
        std::cout << "m_pShareMemOutput = " << &m_pShareMemOutput << std::endl;
        std::cout << "create output share mem success." << std::endl;
        //system("pause");
    }
#else 
    key_t key = ftok(SHAREMEM_FILEPATH_OUTPUT,2);
    if(key == -1)
    {
        std::cout << "createOutputShareMem fail,ftok fail." << std::endl;
        return false;
    }
    std::cout << "createOutputShareMem ,ftok success." << std::endl;
    m_shareMemOutputHandle = shmget(key,SHAREMEM_FILE_SIZE , IPC_CREAT | 0666);
    if(m_shareMemOutputHandle == -1)
    {
        std::cout << "createOutputShareMem fail,shmget fail." << std::endl;
        return false;
    }
    std::cout << "createOutputShareMem ,shmget success." << std::endl;
    m_pShareMemOutput = (char*)shmat(m_shareMemOutputHandle,(void*)0,0);
    std::cout << "createOutputShareMem ,shmat success." << std::endl;
#endif
    memset(m_pShareMemOutput,'\0',SHAREMEM_FILE_SIZE);
    *(m_pShareMemOutput + (MONITOR_TUPPU_CHANGE_POSITION * 2) + 1) = 0x81;
    return true;
}

bool tcpDemo::createShareMem()
{
    if(!createInputShareMem() || !createOutputShareMem())
        return false;
    return true;
}

void tcpDemo::MonitorInputShareMem()
{
    if(m_pShareMemInput == NULL)
    {
        std::cout << "m_pShareMemInput = Null,Please restart" << std::endl;
        MyPause();
        return;
    }
    char* CommndTemp = new char[PER_COMMND_SIZE + 1];
    while(1)
    {
        if(m_E_CommndsocketStatus != socket_status_connected)
        {
            ReconnectSlave();
            continue;
        }
        if(*(m_pShareMemInput + SHAREMEM_INPUT_FLAG) != 0x01)
            continue;
        char * temp = m_pShareMemInput;
        if(CommndTemp == NULL)
            continue;
        bool bCheckFinish = false;
        for(int i = 0;i < (SHAREMEM_FILE_SIZE / PER_COMMND_SIZE);i++)
        {
            if(i == (SHAREMEM_FILE_SIZE / PER_COMMND_SIZE) - 1)
                bCheckFinish = true;
            bool bDataChanged = false;
            memcpy(CommndTemp,(temp + (i * PER_COMMND_SIZE)),PER_COMMND_SIZE);
            *(CommndTemp + PER_COMMND_SIZE + 1) = '\0';
            for(auto i = 0;i < PER_COMMND_SIZE;i++)
            {
                if(CommndTemp[i] != (int)0)
                {
                    bDataChanged = true;
                    //break;
                }
            }
            if(!bDataChanged)
            {
                //std::cout << "Monitor input share mem not changed,i = " << i << std::endl;
                continue;
            }
#ifdef _WIN32
            int send_len = send(m_socketfd, CommndTemp, PER_COMMND_SIZE, 0);
#else
            int send_len = send(m_socketfd, CommndTemp, PER_COMMND_SIZE , 0);//向TCP连接的另一端发送数据。
#endif
            long long lltime = getTimeInMS();
            std::cout << "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,send Commnd finish time: " << lltime << std::endl;
            if(send_len < 0)
            {
                std::cout << "Monitor input share mem send data fail" << std::endl;
                m_E_CommndsocketStatus = socket_status_disconnect;
                break;
            }
            else
            {
                std::cout << "Monitor input share mem send data success" << std::endl;
                for(int j = 0;j < PER_COMMND_SIZE;j++)
                    std::cout << std::hex << (unsigned int)(*(CommndTemp + j)) << "  ";
                std::cout << std::endl;
            }
            memset((temp + (i * PER_COMMND_SIZE)),'\0',PER_COMMND_SIZE);
        }
        if(bCheckFinish)
            *(m_pShareMemInput + SHAREMEM_INPUT_FLAG) = 0x00;
    }
    delete []CommndTemp;
}

void tcpDemo::MonitorTerminal()
{
    if(m_pShareMemOutput == NULL)
    {
        std::cout << "MonitorTerminal fail,share mem output is null,please restart." << std::endl;
        MyPause();
        return;
    }
    int totalLength = (TERMINAL_DATA_SIMULINK_LENGTH + m_ui_NodeYaocePointsNumber * m_ui_NodesNumber);
    int iRequestBeginCal = 0;
    int iEachLength = totalLength - iRequestBeginCal > OUTPUT_DATA_PER_LENGTH ? 
                        OUTPUT_DATA_PER_LENGTH : totalLength - iRequestBeginCal;
    std::string strCommnd(12,0);
    setRequestCommnd(strCommnd,iRequestBeginCal + ALL_DATA_BEGIN_POSITION,iEachLength);
    char* buffer = new char[SOCKET_RECV_BUF_SIZE];
    while(1)
    {
        if(m_E_MonitorSocketStatus != socket_status_connected)
        {
            ReconnectSlave();
            continue;
        }
        memset(buffer,0,SOCKET_RECV_BUF_SIZE);
        if(!sendCommndAndRecv(m_socketlistenfd,strCommnd,buffer,SOCKET_RECV_BUF_SIZE))
        {
            m_E_MonitorSocketStatus = socket_status_disconnect;
            ReconnectSlave();
            continue;
        }
        if(IsTuopuChanged(m_TuopoFlag,buffer + 9,TUOPO_FLAG_SIZE))
        //if(((*(buffer + 9 + 1) & 0x01) != m_cTuopoTerminalStatus))
        {
            //update tuopo status
            memcpy(m_TuopoFlag,buffer + 9,TUOPO_FLAG_SIZE);
            if(m_bInitTuopoFlag)
            {
                m_bInitTuopoFlag = false;
                continue;
            }
            if((int)(*(m_pShareMemOutput + SHAREMEM_OUTPUT_FLAG)) == 1)
                memcpy(m_pShareMemOutput + ((iRequestBeginCal + ALL_DATA_BEGIN_POSITION) * 2),buffer + 9,iEachLength * 2);
            long long lltime = getTimeInMS();
            std::cout << "aaaaaaaaaaaaaaaaaaaaaaaaaaaaa,tuopo changed time = " << lltime << ",tuopo flag = ";
            for(auto i = 0;i < TUOPO_FLAG_SIZE;i++)
                std::cout << std::hex << (unsigned int)m_TuopoFlag[i] << ",";
            std::cout << std::endl;
            {
                std::unique_lock<std::recursive_mutex> lock(m_TuopoMutex);
                m_bTuopoStatusChanged = true;
            }
            getAllData(true);
        }
        //update tuopo status to java
        memcpy(m_pShareMemOutput + (MONITOR_TUPPU_CHANGE_POSITION * 2),buffer + 9,TUOPO_FLAG_SIZE);
    }
    delete []buffer;
    buffer = NULL;
}

void tcpDemo::setRequestCommnd(std::string &strCommnd,int iBeginPos,int iLength)
{
    if(strCommnd.size() != PER_COMMND_SIZE)
    {
        std::cout << "set Request Commnd fail,strCommnd size != " << PER_COMMND_SIZE << std::endl;
        return;
    }
    strCommnd[REQUEST_COMMND_REQUESTSIZE_POSITION] = (int)6;
    strCommnd[6] = (int)1;
    strCommnd[7] = (int)3;
    strCommnd[8] = (int)(iBeginPos / 256);
    strCommnd[9] = (int)(iBeginPos % 256);
    strCommnd[10] = (int)(iLength / 256);
    strCommnd[11] = (int)(iLength % 256);
}

void tcpDemo::MonitorTuoPu()
{
    if(!bInitStatus)
        return;
    std::ofstream outputFile;
    outputFile.open(MONITOR_TUPPU_FILE_PATH, std::fstream::out | std::ios::app | std::ios::ate);
    if(!outputFile.is_open())
    {
        std::cout << "Monitor Tuopu fail,open file fail." << std::endl;
        return;
    }
    if(m_pShareMemOutput == NULL)
    {
        std::cout << "Monitor Tuopu fail,m_pShareMemOutput = NULL." << std::endl;
        return;
    }
    std::cout << "start to monitor tuopu position : " << (int)MONITOR_TUPPU_CHANGE_POSITION << std::endl;
    while(1)
    {
        MySleep(2);
        if(m_bTuopoStatusChanged)
        {
            long long lltime = getTimeInMS();
            outputFile << "time = " << lltime << ",tuopu new status = " << m_cTuopoTerminalStatus << std::endl;
            outputFile.flush();
            {
                std::unique_lock<std::recursive_mutex> lock(m_TuopoMutex);
                m_bTuopoStatusChanged = false;
            }
        }
    }
    outputFile.close();
}

#ifdef _WIN32
bool tcpDemo::initialization()
{
    //初始化套接字库
	WORD w_req = MAKEWORD(2, 2);//版本号
	WSADATA wsadata;
	int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) 
    {
		std::cout << "初始化套接字库失败！" << std::endl;
        return false;
	}
	std::cout << "初始化套接字库成功！" << std::endl;
	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		std::cout << "套接字库版本号不符！" << std::endl;
		WSACleanup();
        return false;
	}
	std::cout << "套接字库版本正确！" << std::endl;
    return true;
}
#endif

bool tcpDemo::IsTuopuChanged(char* oldStatus,char* newStatus,int iSize)
{
    if(oldStatus == NULL || newStatus == NULL)
        return false;
    if(iSize <= 0)
        return false;
    for(auto i = 0;i < iSize;i++)
    {
        if(oldStatus[i] != newStatus[i])
            return true;
    }
    return false;
}