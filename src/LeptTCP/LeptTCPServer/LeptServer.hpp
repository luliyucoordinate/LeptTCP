#ifndef _LEPTSERVER_HPP_
#define _LEPTSERVER_HPP_

#include "Lept.hpp"
#include "INetEvent.hpp"

#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <memory>

//������Ϣ��������
class LeptS2LTask : public LeptTask
{
    LeptClientPtr _pClient;
    DataHeaderPtr _pHeader;
public:
    LeptS2LTask(LeptClientPtr& pClient, DataHeaderPtr& header)
    {
        _pClient = pClient;
        _pHeader = header;
    }

    //ִ������
    void doTask()
    {
        _pClient->SendData(_pHeader);
    }
};
typedef std::shared_ptr<LeptS2LTask> LeptS2LTaskPtr;

//������Ϣ���մ���������
class LeptServer
{
public:
    LeptServer(SOCKET sock = INVALID_SOCKET)
    {
        _sock = sock;
        _pNetEvent = nullptr;
    }

    ~LeptServer()
    {
        Close();
        _sock = INVALID_SOCKET;
    }

    void setEventObj(INetEvent* event)
    {
        _pNetEvent = event;
    }

    //�ر�Socket
    void Close()
    {
        if (_sock != INVALID_SOCKET)
        {
#ifdef _WIN32
            for (auto iter : _clients)
            {
                closesocket(iter.second->sockfd());
            }
            //�ر��׽���closesocket
            closesocket(_sock);
#else
            for (auto iter : _clients)
            {
                close(iter.second->sockfd());
            }
            //�ر��׽���closesocket
            close(_sock);
#endif
            _clients.clear();
        }
    }

    //�Ƿ�����
    bool isRun()
    {
        return _sock != INVALID_SOCKET;
    }

    //����������Ϣ
    //���ݿͻ�socket fd_set
    fd_set _fdRead_bak;
    //�ͻ��б��Ƿ��б仯
    bool _clients_change;
    SOCKET _maxSock;
    void OnRun()
    {
        _clients_change = true;
        while (isRun())
        {
            if (!_clientsBuff.empty())
            {//�ӻ��������ȡ���ͻ�����
                std::lock_guard<std::mutex> lock(_mutex);
                for (auto pClient : _clientsBuff)
                {
                    _clients[pClient->sockfd()] = pClient;
                }
                _clientsBuff.clear();
                _clients_change = true;
            }

            //���û����Ҫ�����Ŀͻ��ˣ�������
            if (_clients.empty())
            {
                std::chrono::milliseconds t(1);
                std::this_thread::sleep_for(t);
                continue;
            }

            //�������׽��� BSD socket
            fd_set fdRead;//��������socket�� ����
                          //��������
            FD_ZERO(&fdRead);
            if (_clients_change)
            {
                _clients_change = false;
                //����������socket�����뼯��
                _maxSock = _clients.begin()->second->sockfd();
                for (auto iter : _clients)
                {
                    FD_SET(iter.second->sockfd(), &fdRead);
                    if (_maxSock < iter.second->sockfd())
                    {
                        _maxSock = iter.second->sockfd();
                    }
                }
                memcpy(&_fdRead_bak, &fdRead, sizeof(fd_set));
            }
            else {
                memcpy(&fdRead, &_fdRead_bak, sizeof(fd_set));
            }

            //nfds ��һ������ֵ ��ָfd_set����������������(socket)�ķ�Χ������������
            //���������ļ����������ֵ+1 ��Windows�������������д0
            int ret = select(_maxSock + 1, &fdRead, nullptr, nullptr, nullptr);
            if (ret < 0)
            {
                printf("select���������\n");
                Close();
                return;
            }
            else if (ret == 0)
            {
                continue;
            }

#ifdef _WIN32
            for (int n = 0; n < fdRead.fd_count; n++)
            {
                auto iter = _clients.find(fdRead.fd_array[n]);
                if (iter != _clients.end())
                {
                    if (-1 == RecvData(iter->second))
                    {
                        if (_pNetEvent)
                            _pNetEvent->OnNetLeave(iter->second);
                        _clients_change = true;
                        _clients.erase(iter->first);
                    }
                }
                else {
                    printf("error. if (iter != _clients.end())\n");
                }

            }
#else
            std::vector<LeptClientPtr> temp;
            for (auto iter : _clients)
            {
                if (FD_ISSET(iter.second->sockfd(), &fdRead))
                {
                    if (-1 == RecvData(iter.second))
                    {
                        if (_pNetEvent)
                            _pNetEvent->OnNetLeave(iter.second);
                        _clients_change = false;
                        temp.push_back(iter.second);
                    }
                }
            }
            for (auto pClient : temp)
            {
                _clients.erase(pClient->sockfd());
            }
#endif
        }
    }
    //�������� ����ճ�� ��ְ�
    int RecvData(LeptClientPtr& pClient)
    {

        //���տͻ�������
        char* szRecv = pClient->msgBuf() + pClient->getLastPos();
        int nLen = (int)recv(pClient->sockfd(), szRecv, (RECV_BUFF_SZIE)-pClient->getLastPos(), 0);
        _pNetEvent->OnNetRecv(pClient);
        //printf("nLen=%d\n", nLen);
        if (nLen <= 0)
        {
            //printf("�ͻ���<Socket=%d>���˳������������\n", pClient->sockfd());
            return -1;
        }
        //����ȡ�������ݿ�������Ϣ������
        //memcpy(pClient->msgBuf() + pClient->getLastPos(), _szRecv, nLen);
        //��Ϣ������������β��λ�ú���
        pClient->setLastPos(pClient->getLastPos() + nLen);

        //�ж���Ϣ�����������ݳ��ȴ�����ϢͷDataHeader����
        while (pClient->getLastPos() >= sizeof(DataHeader))
        {
            //��ʱ�Ϳ���֪����ǰ��Ϣ�ĳ���
            DataHeader* header = (DataHeader*)pClient->msgBuf();
            //�ж���Ϣ�����������ݳ��ȴ�����Ϣ����
            if (pClient->getLastPos() >= header->dataLength)
            {
                //��Ϣ������ʣ��δ�������ݵĳ���
                int nSize = pClient->getLastPos() - header->dataLength;
                //����������Ϣ
                OnNetMsg(pClient, header);
                //����Ϣ������ʣ��δ��������ǰ��
                memcpy(pClient->msgBuf(), pClient->msgBuf() + header->dataLength, nSize);
                //��Ϣ������������β��λ��ǰ��
                pClient->setLastPos(nSize);
            }
            else {
                //��Ϣ������ʣ�����ݲ���һ��������Ϣ
                break;
            }
        }
        return 0;
    }

    //��Ӧ������Ϣ
    virtual void OnNetMsg(LeptClientPtr& pClient, DataHeader* header)
    {
        _pNetEvent->OnNetMsg(this, pClient, header);
    }

    void addClient(LeptClientPtr& pClient)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        //_mutex.lock();
        _clientsBuff.push_back(pClient);
        //_mutex.unlock();
    }

    void Start()
    {
        _thread = std::thread(std::mem_fn(&LeptServer::OnRun), this);
        _taskServer.Start();
    }

    size_t getClientCount()
    {
        return _clients.size() + _clientsBuff.size();
    }

    void addSendTask(LeptClientPtr& pClient, DataHeaderPtr& header)
    {
        auto task = std::make_shared<LeptS2LTask>(pClient, header);
        _taskServer.addTask((LeptTaskPtr)task);
    }
private:
    SOCKET _sock;
    //��ʽ�ͻ�����
    std::map<SOCKET, LeptClientPtr> _clients;
    //����ͻ�����
    std::vector<LeptClientPtr> _clientsBuff;
    //������е���
    std::mutex _mutex;
    std::thread _thread;
    //�����¼�����
    INetEvent* _pNetEvent;
    //
    LeptTaskServer _taskServer;
};

#endif // !_LEPTSERVER_HPP_