#ifndef _LEPTCLIENT_HPP_
#define _LEPTCLIENT_HPP_

#include "Lept.hpp"

typedef std::shared_ptr<DataHeader> DataHeaderPtr;
typedef std::shared_ptr<LoginResult> LoginResultPtr;


//��������С��Ԫ��С
#ifndef RECV_BUFF_SZIE
#define RECV_BUFF_SZIE 10240*5
#define SEND_BUFF_SZIE RECV_BUFF_SZIE
#endif // !RECV_BUFF_SZIE

//�ͻ�����������
class LeptClient : public ObjectPoolBase<LeptClient, 10000>
{
public:
    LeptClient(SOCKET sockfd = INVALID_SOCKET)
    {
        _sockfd = sockfd;
        memset(_szMsgBuf, 0, RECV_BUFF_SZIE);
        _lastPos = 0;

        memset(_szSendBuf, 0, SEND_BUFF_SZIE);
        _lastSendPos = 0;
    }

    SOCKET sockfd()
    {
        return _sockfd;
    }

    char* msgBuf()
    {
        return _szMsgBuf;
    }

    int getLastPos()
    {
        return _lastPos;
    }
    void setLastPos(int pos)
    {
        _lastPos = pos;
    }

    //��������
    int SendData(DataHeaderPtr& header)
    {
        int ret = SOCKET_ERROR;
        //Ҫ���͵����ݳ���
        int nSendLen = header->dataLength;
        //Ҫ���͵�����
        const char* pSendData = (const char*)header.get();

        while (true)
        {
            if (_lastSendPos + nSendLen >= SEND_BUFF_SZIE)
            {
                //����ɿ��������ݳ���
                int nCopyLen = SEND_BUFF_SZIE - _lastSendPos;
                //��������
                memcpy(_szSendBuf + _lastSendPos, pSendData, nCopyLen);
                //����ʣ������λ��
                pSendData += nCopyLen;
                //����ʣ�����ݳ���
                nSendLen -= nCopyLen;
                //��������
                ret = send(_sockfd, _szSendBuf, SEND_BUFF_SZIE, 0);
                //����β��λ������
                _lastSendPos = 0;
                //���ʹ���
                if (SOCKET_ERROR == ret)
                {
                    return ret;
                }
            }
            else {
                //��Ҫ���͵����� ���������ͻ�����β��
                memcpy(_szSendBuf + _lastSendPos, pSendData, nSendLen);
                //��������β��λ��
                _lastSendPos += nSendLen;
                break;
            }
        }
        return ret;
    }

private:
    // socket fd_set  file desc set
    SOCKET _sockfd;
    //�ڶ������� ��Ϣ������
    char _szMsgBuf[RECV_BUFF_SZIE];
    //��Ϣ������������β��λ��
    int _lastPos;

    //�ڶ������� ���ͻ�����
    char _szSendBuf[SEND_BUFF_SZIE];
    //���ͻ�����������β��λ��
    int _lastSendPos;
};

typedef std::shared_ptr<LeptClient> LeptClientPtr;

#endif // !_LEPTCLIENT_HPP_
