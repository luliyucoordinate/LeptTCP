#include "Allocator.h"
#include "LeptTCPServer.hpp"
#include<thread>

bool g_bRun = true;
void cmdThread()
{//
    while (true)
    {
        char cmdBuf[256] = {};
        scanf("%s", cmdBuf);
        if (0 == strcmp(cmdBuf, "exit"))
        {
            g_bRun = false;
            printf("�˳�cmdThread�߳�\n");
            break;
        }
        else {
            printf("��֧�ֵ����\n");
        }
    }
}

class MyServer : public LeptTCPServer
{
public:

    //ֻ�ᱻһ���̴߳��� ��ȫ
    virtual void OnNetJoin(ClientSocketPtr& pClient)
    {
        LeptTCPServer::OnNetJoin(pClient);
    }
    //cellServer 4 ����̴߳��� ����ȫ
    //���ֻ����1��cellServer���ǰ�ȫ��
    virtual void OnNetLeave(ClientSocketPtr& pClient)
    {
        LeptTCPServer::OnNetLeave(pClient);
    }
    //cellServer 4 ����̴߳��� ����ȫ
    //���ֻ����1��cellServer���ǰ�ȫ��
    virtual void OnNetMsg(LeptServer* pCellServer, ClientSocketPtr& pClient, DataHeader* header)
    {
        LeptTCPServer::OnNetMsg(pCellServer, pClient, header);
        switch (header->cmd)
        {
        case CMD_LOGIN:
        {
            //send recv 
            Login* login = (Login*)header;
            //printf("�յ��ͻ���<Socket=%d>����CMD_LOGIN,���ݳ��ȣ�%d,userName=%s PassWord=%s\n", pClient->sockfd(), login->dataLength, login->userName, login->PassWord);
            //�����ж��û������Ƿ���ȷ�Ĺ���
            //LoginResult ret;
            //pClient->SendData(&ret);
            auto ret = std::make_shared<LoginResult>();
            pCellServer->addSendTask(pClient, (DataHeaderPtr)ret);
        }//���� ��Ϣ---���� ����   ������ ���ݻ�����  ������ 
        break;
        case CMD_LOGOUT:
        {
            Logout* logout = (Logout*)header;
            //printf("�յ��ͻ���<Socket=%d>����CMD_LOGOUT,���ݳ��ȣ�%d,userName=%s \n", cSock, logout->dataLength, logout->userName);
            //�����ж��û������Ƿ���ȷ�Ĺ���
            //LogoutResult ret;
            //SendData(cSock, &ret);
        }
        break;
        default:
        {
            printf("<socket=%d>�յ�δ������Ϣ,���ݳ��ȣ�%d\n", pClient->sockfd(), header->dataLength);
            //DataHeader ret;
            //SendData(cSock, &ret);
        }
        break;
        }
    }
private:

};

int main()
{

    MyServer server;
    server.InitSocket();
    server.Bind(nullptr, 14567);
    server.Listen(5);
    server.Start(4);

    //����UI�߳�
    std::thread t1(cmdThread);
    t1.detach();

    while (g_bRun)
    {
        server.OnRun();
        //printf("����ʱ�䴦������ҵ��..\n");
    }
    server.Close();
    printf("���˳���\n");
    getchar();
    return 0;
}