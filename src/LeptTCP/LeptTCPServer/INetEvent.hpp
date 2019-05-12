#ifndef _I_NET_EVENT_HPP_
#define _I_NET_EVENT_HPP_

#include "Lept.hpp"
#include "LeptClient.hpp"
class LeptServer;

//网络事件接口
class INetEvent
{
public:
    //纯虚函数
    //客户端加入事件
    virtual void OnNetJoin(LeptClientPtr& pClient) = 0;
    //客户端离开事件
    virtual void OnNetLeave(LeptClientPtr& pClient) = 0;
    //客户端消息事件
    virtual void OnNetMsg(LeptServer* pLeptServer, LeptClientPtr& pClient, DataHeader* header) = 0;
    //recv事件
    virtual void OnNetRecv(LeptClientPtr& pClient) = 0;
private:

};

#endif // !_INETEVENT_HPP_
