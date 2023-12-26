// Mini Redis C++ publisher and subscriber

#include <iostream>
#include <sstream>
#include <string.h>
#include <thread>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>
#include "MiniRedisPubSub.h"

MiniRedisPubSub::MiniRedisPubSub()
{
    Init();
}

MiniRedisPubSub::~MiniRedisPubSub()
{
    Disconnect();
}

void MiniRedisPubSub::Init()
{
    host = "127.0.0.1";
    port = 6379;
    asyncContext = nullptr;
    evt = nullptr;
    subCb = nullptr; 
}

void MiniRedisPubSub::SetHost(const std::string& host)
{
    this->host = host;
}

std::string MiniRedisPubSub::GetHost() const
{
    return host;
}

void MiniRedisPubSub::SetPort(uint16_t port)
{
    this->port = port;
}

uint16_t MiniRedisPubSub::GetPort() const
{
    return port;
}

// Will be called when data is received from subscribed channels
void MiniRedisPubSub::SetSubscribeCb(SubscribeCbFunc cb)
{
    subCb = cb; 
}

bool MiniRedisPubSub::Connect()
{
    // Async Connect will return at once, need to check result at OnConnect callback
    asyncContext = redisAsyncConnect(host.c_str(), port); 
    if (!asyncContext || asyncContext->err)
    {
        if (asyncContext)
        {
            std::cerr << "Failed to connect to Redis server: " << asyncContext->errstr << std::endl;
        }
        else
        {
            std::cerr << "Can't allocate redis context" << std::endl;
        }

        return false;
    }

    // Attach libevent to context
    if (!evt)
    {
        evt = event_base_new(); 
    }
    redisLibeventAttach(asyncContext, evt);

    redisAsyncSetConnectCallback(asyncContext, OnConnect);  
    redisAsyncSetDisconnectCallback(asyncContext, OnDisconnect); 

    // Create event loop thread
    std::thread t(ThreadRoutine, this);
    if (t.joinable())
    {
        // Let libevent to handle it
        t.detach();
    } 

    return true;
}

bool MiniRedisPubSub::Connect(const std::string& host, uint16_t port)
{
    this->host = host;
    this->port = port;

    return Connect();
}

bool MiniRedisPubSub::Disconnect()
{
    if (!asyncContext)
    {
        return false;
    }

    // This function will free context
    redisAsyncDisconnect(asyncContext);
    asyncContext = nullptr;

    // Break event thread loop
    event_base_loopbreak(evt);
    event_base_free(evt);
    evt = nullptr; 

    return true; 
}

bool MiniRedisPubSub::Publish(const std::string& channel, const std::string& content)
{
    if (!asyncContext || channel.empty() || content.empty())
    {
        return false;
    }

    int ret = redisAsyncCommand(asyncContext, OnPublishMsg, this, 
        "PUBLISH %b %b", 
        channel.c_str(), channel.size(), 
        content.c_str(), content.size());
    if (ret == REDIS_ERR)
    {
        std::cerr << "Failed to publish to " << channel << std::endl; 
        return false; 
    }

    return true; 
}

bool MiniRedisPubSub::Subscribe(const std::string& channel)
{
    if (!asyncContext || channel.empty())
    {
        return false;
    }

    int ret = redisAsyncCommand(asyncContext, OnSubscribeMsg, this, 
        "SUBSCRIBE %b", 
        channel.c_str(), channel.size());
    if (ret == REDIS_ERR)
    {
        std::cerr << "Failed to subscribe from " << channel << std::endl; 
        return false; 
    }

    return true; 
}

bool MiniRedisPubSub::Unsubscribe(const std::string& channel)
{
    if (!asyncContext || channel.empty())
    {
        return false;
    }

    // Use the same callback as subscribe
    int ret = redisAsyncCommand(asyncContext, OnSubscribeMsg, this, 
        "UNSUBSCRIBE %b", 
        channel.c_str(), channel.size());
    if (ret == REDIS_ERR)
    {
        std::cerr << "Failed to unsubscribe from " << channel << std::endl; 
        return false; 
    }

    return true; 
}

void MiniRedisPubSub::ThreadRoutine(void* arg)
{
    MiniRedisPubSub* pThis = reinterpret_cast<MiniRedisPubSub*>(arg);
    if (!pThis)
    {
        return; 
    }

    // Run the libevent loop inside thread
    event_base_dispatch(pThis->evt); 
    //std::cout << "libevent thread ends" << std::endl; 
}

void MiniRedisPubSub::OnConnect(const redisAsyncContext* ac, int status) 
{ 
    if (status != REDIS_OK) 
    { 
        std::cerr << "Failed to connect to Redis. Error: " << status << ", description: " << ac->errstr << std::endl; 
    }
    else
    {
        std::cout << "Redis async connected" << std::endl; 
    }
}

void MiniRedisPubSub::OnDisconnect(const redisAsyncContext *ac, int status) 
{
    // Can reconnect here
    if (status != REDIS_OK) 
    {  
        std::cerr << "Redis disconnected abnormally. Error: " << status << ", description: " << ac->errstr << std::endl; 
    }
    else
    {
        std::cout << "Redis disconnected" << std::endl; 
    }
} 

// Callback when the data is published
void MiniRedisPubSub::OnPublishMsg(redisAsyncContext*, void*, void*)
{
    //std::cout << "Message published" << std::endl; 
    return; 
}

// Callback when: subscribe channel, unsubscribe channel, receive data from channel
void MiniRedisPubSub::OnSubscribeMsg(redisAsyncContext*, void* replyData, void* privData)
{
    MiniRedisPubSub* pThis = reinterpret_cast<MiniRedisPubSub*>(privData);
    redisReply* reply = reinterpret_cast<redisReply*>(replyData);
    if (!pThis || !reply) 
    {
        return;
    }

    // It should be an array with 3 elements
    // element 0: "message"/"subscribe"/"unsubscribe"
    // element 1: the channel name
    // element 2: the content published
    if (reply->type != REDIS_REPLY_ARRAY)
    {
        std::cerr << "Expecting array while receiving " << reply->type << std::endl;
        return;
    }
    if (reply->elements != 3)
    {
        std::cerr << "Expecting 3 elements in array while receiving " << reply->elements << std::endl;
        return;
    }

    // Get type of the reply: message, subscribe ACK, or unsubscribe ACK
    redisReply* elem = reply->element[0]; 
    std::string replyType(elem->str, elem->len);
    if (replyType == "subscribe")
    {
        std::cout << "Subscribe ACK" << std::endl;
    }
    else if (replyType == "unsubscribe")
    {
        std::cout << "Unsubscribe ACK" << std::endl;
    }
    else
    {
        redisReply* elemChannel = reply->element[1]; 
        std::string channel(elemChannel->str, elemChannel->len);
        redisReply* elemContent = reply->element[2]; 
        std::string content(elemContent->str, elemContent->len);
        if (pThis->subCb)
        {
            pThis->subCb(channel, content);
        }
        else
        {
            std::cout << "Subscribe channel: " << channel << ", content: " << content << std::endl; 
        }
    }
}
