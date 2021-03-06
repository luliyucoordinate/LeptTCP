#ifndef _LEPTTCPSERVER_HPP_
#define _LEPTTCPSERVER_HPP_


#include "Lept.hpp"
#include "LeptClient.hpp"
#include "LeptServer.hpp"
#include "INetEvent.hpp"
#include <stdio.h>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>

typedef std::shared_ptr<LeptServer> LeptServerPtr;
class LeptTCPServer : public INetEvent
{
private:
    SOCKET _sock;
    //消息处理对象，内部会创建线程
    std::vector<LeptServerPtr> _leptServers;
    //每秒消息计时
    LeptTimeStamp _tTime;
protected:
    //SOCKET recv计数
    std::atomic_int _recvCount;
    //收到消息计数
    std::atomic_int _msgCount;
    //客户端计数
    std::atomic_int _clientCount;
public:
    LeptTCPServer()
    {
        _sock = INVALID_SOCKET;
        _recvCount = 0;
        _msgCount = 0;
        _clientCount = 0;
    }
    virtual ~LeptTCPServer()
    {
        Close();
    }
    //初始化Socket
    SOCKET InitSocket()
    {
#ifdef _WIN32
        //启动Windows socket 2.x环境
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        if (INVALID_SOCKET != _sock)
        {
            printf("<socket=%d>关闭旧连接...\n", (int)_sock);
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            printf("错误，建立socket失败...\n");
        }
        else {
            printf("建立socket=<%d>成功...\n", (int)_sock);
        }
        return _sock;
    }

    //绑定IP和端口号
    int Bind(const char* ip, unsigned short port)
    {
        //if (INVALID_SOCKET == _sock)
        //{
        //	InitSocket();
        //}
        // 2 bind 绑定用于接受客户端连接的网络端口
        sockaddr_in _sin = {};
        _sin.sin_family = AF_INET;
        _sin.sin_port = htons(port);//host to net unsigned short

#ifdef _WIN32
        if (ip) {
            _sin.sin_addr.S_un.S_addr = inet_addr(ip);
        }
        else {
            _sin.sin_addr.S_un.S_addr = INADDR_ANY;
        }
#else
        if (ip) {
            _sin.sin_addr.s_addr = inet_addr(ip);
        }
        else {
            _sin.sin_addr.s_addr = INADDR_ANY;
        }
#endif
        int ret = bind(_sock, (sockaddr*)&_sin, sizeof(_sin));
        if (SOCKET_ERROR == ret)
        {
            printf("错误,绑定网络端口<%d>失败...\n", port);
        }
        else {
            printf("绑定网络端口<%d>成功...\n", port);
        }
        return ret;
    }

    //监听端口号
    int Listen(int n)
    {
        // 3 listen 监听网络端口
        int ret = listen(_sock, n);
        if (SOCKET_ERROR == ret)
        {
            printf("socket=<%d>错误,监听网络端口失败...\n", _sock);
        }
        else {
            printf("socket=<%d>监听网络端口成功...\n", _sock);
        }
        return ret;
    }

    //接受客户端连接
    SOCKET Accept()
    {
        // 4 accept 等待接受客户端连接
        sockaddr_in clientAddr = {};
        int nAddrLen = sizeof(sockaddr_in);
        SOCKET cSock = INVALID_SOCKET;
#ifdef _WIN32
        cSock = accept(_sock, (sockaddr*)&clientAddr, &nAddrLen);
#else
        cSock = accept(_sock, (sockaddr*)&clientAddr, (socklen_t *)&nAddrLen);
#endif
        if (INVALID_SOCKET == cSock)
        {
            printf("socket=<%d>错误,接受到无效客户端SOCKET...\n", (int)_sock);
        }
        else
        {
            //将新客户端分配给客户数量最少的LeptServer
            LeptClientPtr c(new LeptClient(cSock));
            //addClientToLeptServer(std::make_shared<LeptClient>(cSock));
            addClientToLeptServer(c);
            //获取IP地址 inet_ntoa(clientAddr.sin_addr)
        }
        return cSock;
    }

    void addClientToLeptServer(LeptClientPtr& pClient)
    {
        //查找客户数量最少的LeptServer消息处理对象
        auto pMinServer = _leptServers[0];
        for (auto pLeptServer : _leptServers)
        {
            if (pMinServer->getClientCount() > pLeptServer->getClientCount())
            {
                pMinServer = pLeptServer;
            }
        }
        pMinServer->addClient(pClient);
        OnNetJoin(pClient);
    }

    void Start(int nLeptServer)
    {
        for (int n = 0; n < nLeptServer; n++)
        {
            auto ser = std::make_shared<LeptServer>(_sock);
            _leptServers.push_back(ser);
            //注册网络事件接受对象
            ser->setEventObj(this);
            //启动消息处理线程
            ser->Start();
        }
    }
    //关闭Socket
    void Close()
    {
        if (_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            //关闭套节字closesocket
            closesocket(_sock);
            //------------
            //清除Windows socket环境
            WSACleanup();
#else
            //关闭套节字closesocket
            close(_sock);
#endif
        }
    }
    //处理网络消息
    bool OnRun()
    {
        if (isRun())
        {
            time4msg();
            //伯克利套接字 BSD socket
            fd_set fdRead;//描述符（socket） 集合
                          //清理集合
            FD_ZERO(&fdRead);
            //将描述符（socket）加入集合
            FD_SET(_sock, &fdRead);
            //nfds 是一个整数值 是指fd_set集合中所有描述符(socket)的范围，而不是数量
            //既是所有文件描述符最大值+1 在Windows中这个参数可以写0
            timeval t = { 0,10 };
            int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
            if (ret < 0)
            {
                printf("Accept Select任务结束。\n");
                Close();
                return false;
            }
            //判断描述符（socket）是否在集合中
            if (FD_ISSET(_sock, &fdRead))
            {
                FD_CLR(_sock, &fdRead);
                Accept();
                return true;
            }
            return true;
        }
        return false;
    }
    //是否工作中
    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

    //计算并输出每秒收到的网络消息
    void time4msg()
    {
        auto t1 = _tTime.getElapsedSecond();
        if (t1 >= 1.0)
        {
            printf("thread<%d>,time<%lf>,socket<%d>,clients<%d>,recv<%d>,msg<%d>\n", _leptServers.size(), t1, _sock, (int)_clientCount, (int)(_recvCount / t1), (int)(_msgCount / t1));
            _recvCount = 0;
            _msgCount = 0;
            _tTime.update();
        }
    }
    //只会被一个线程触发 安全
    virtual void OnNetJoin(LeptClientPtr& pClient)
    {
        _clientCount++;
        //printf("client<%d> join\n", pClient->sockfd());
    }
    //LeptServer 4 多个线程触发 不安全
    //如果只开启1个LeptServer就是安全的
    virtual void OnNetLeave(LeptClientPtr& pClient)
    {
        _clientCount--;
        //printf("client<%d> leave\n", pClient->sockfd());
    }
    //LeptServer 4 多个线程触发 不安全
    //如果只开启1个LeptServer就是安全的
    virtual void OnNetMsg(LeptServer* pLeptServer, LeptClientPtr& pClient, DataHeader* header)
    {
        _msgCount++;
    }

    virtual void OnNetRecv(LeptClientPtr& pClient)
    {
        _recvCount++;
        //printf("client<%d> leave\n", pClient->sockfd());
    }
};

#endif // !_LEPTTCPSERVER_HPP_
