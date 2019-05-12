#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include "Lept.hpp"
#include "LeptClient.hpp"
class LeptServer;

//�����¼��ӿ�
class INetEvent
{
public:
    //���麯��
    //�ͻ��˼����¼�
    virtual void OnNetJoin(LeptClientPtr& pClient) = 0;
    //�ͻ����뿪�¼�
    virtual void OnNetLeave(LeptClientPtr& pClient) = 0;
    //�ͻ�����Ϣ�¼�
    virtual void OnNetMsg(LeptServer* pLeptServer, LeptClientPtr& pClient, DataHeader* header) = 0;
    //recv�¼�
    virtual void OnNetRecv(LeptClientPtr& pClient) = 0;
private:

};

#endif // !_INETEVENT_HPP_
