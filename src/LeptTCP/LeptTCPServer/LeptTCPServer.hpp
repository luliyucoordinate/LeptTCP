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
    //��Ϣ��������ڲ��ᴴ���߳�
    std::vector<LeptServerPtr> _leptServers;
    //ÿ����Ϣ��ʱ
    LeptTimeStamp _tTime;
protected:
    //SOCKET recv����
    std::atomic_int _recvCount;
    //�յ���Ϣ����
    std::atomic_int _msgCount;
    //�ͻ��˼���
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
    //��ʼ��Socket
    SOCKET InitSocket()
    {
#ifdef _WIN32
        //����Windows socket 2.x����
        WORD ver = MAKEWORD(2, 2);
        WSADATA dat;
        WSAStartup(ver, &dat);
#endif
        if (INVALID_SOCKET != _sock)
        {
            printf("<socket=%d>�رվ�����...\n", (int)_sock);
            Close();
        }
        _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (INVALID_SOCKET == _sock)
        {
            printf("���󣬽���socketʧ��...\n");
        }
        else {
            printf("����socket=<%d>�ɹ�...\n", (int)_sock);
        }
        return _sock;
    }

    //��IP�Ͷ˿ں�
    int Bind(const char* ip, unsigned short port)
    {
        //if (INVALID_SOCKET == _sock)
        //{
        //	InitSocket();
        //}
        // 2 bind �����ڽ��ܿͻ������ӵ�����˿�
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
            printf("����,������˿�<%d>ʧ��...\n", port);
        }
        else {
            printf("������˿�<%d>�ɹ�...\n", port);
        }
        return ret;
    }

    //�����˿ں�
    int Listen(int n)
    {
        // 3 listen ��������˿�
        int ret = listen(_sock, n);
        if (SOCKET_ERROR == ret)
        {
            printf("socket=<%d>����,��������˿�ʧ��...\n", _sock);
        }
        else {
            printf("socket=<%d>��������˿ڳɹ�...\n", _sock);
        }
        return ret;
    }

    //���ܿͻ�������
    SOCKET Accept()
    {
        // 4 accept �ȴ����ܿͻ�������
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
            printf("socket=<%d>����,���ܵ���Ч�ͻ���SOCKET...\n", (int)_sock);
        }
        else
        {
            //���¿ͻ��˷�����ͻ��������ٵ�LeptServer
            LeptClientPtr c(new LeptClient(cSock));
            //addClientToLeptServer(std::make_shared<LeptClient>(cSock));
            addClientToLeptServer(c);
            //��ȡIP��ַ inet_ntoa(clientAddr.sin_addr)
        }
        return cSock;
    }

    void addClientToLeptServer(LeptClientPtr& pClient)
    {
        //���ҿͻ��������ٵ�LeptServer��Ϣ�������
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
            //ע�������¼����ܶ���
            ser->setEventObj(this);
            //������Ϣ�����߳�
            ser->Start();
        }
    }
    //�ر�Socket
    void Close()
    {
        if (_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            //�ر��׽���closesocket
            closesocket(_sock);
            //------------
            //���Windows socket����
            WSACleanup();
#else
            //�ر��׽���closesocket
            close(_sock);
#endif
        }
    }
    //����������Ϣ
    bool OnRun()
    {
        if (isRun())
        {
            time4msg();
            //�������׽��� BSD socket
            fd_set fdRead;//��������socket�� ����
                          //������
            FD_ZERO(&fdRead);
            //����������socket�����뼯��
            FD_SET(_sock, &fdRead);
            //nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
            //���������ļ����������ֵ+1 ��Windows�������������д0
            timeval t = { 0,10 };
            int ret = select(_sock + 1, &fdRead, 0, 0, &t); //
            if (ret < 0)
            {
                printf("Accept Select���������\n");
                Close();
                return false;
            }
            //�ж���������socket���Ƿ��ڼ�����
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
    //�Ƿ�����
    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

    //���㲢���ÿ���յ���������Ϣ
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
    //ֻ�ᱻһ���̴߳��� ��ȫ
    virtual void OnNetJoin(LeptClientPtr& pClient)
    {
        _clientCount++;
        //printf("client<%d> join\n", pClient->sockfd());
    }
    //LeptServer 4 ����̴߳��� ����ȫ
    //���ֻ����1��LeptServer���ǰ�ȫ��
    virtual void OnNetLeave(LeptClientPtr& pClient)
    {
        _clientCount--;
        //printf("client<%d> leave\n", pClient->sockfd());
    }
    //LeptServer 4 ����̴߳��� ����ȫ
    //���ֻ����1��LeptServer���ǰ�ȫ��
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
