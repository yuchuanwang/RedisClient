// Mini Redis C++ publisher and subscriber
// Depends on hiredis: 
//   sudo apt install -y libhiredis-dev
// which installs 0.1x version.
// 
// If need new version, please build from source https://github.com/redis/hiredis/
//   git clone https://github.com/redis/hiredis
//   cd hiredis
//   make
//   sudo make install
//   sudo ldconfig /usr/local/lib
//
// Subscriber depends on libevent: 
//   sudo apt -y install libevent-dev
// and link with -levent
// 

#ifndef MiniRedisPubSub_INCLUDED
#define MiniRedisPubSub_INCLUDED

#include <string>
#include <functional>

struct redisAsyncContext;
struct redisReply;
struct event_base;

using SubscribeCbFunc = std::function<void(const std::string&, const std::string&)>;

class MiniRedisPubSub
{
public:
    MiniRedisPubSub();
    ~MiniRedisPubSub();

    // Getter and Setter of this instance
    void SetHost(const std::string& host);
    std::string GetHost() const;
    void SetPort(uint16_t port);
    uint16_t GetPort() const;

    // CB will be called when data is received from subscribed channels
    void SetSubscribeCb(SubscribeCbFunc cb); 

    // Connect to Redis Server
    bool Connect();
    bool Connect(const std::string& host, uint16_t port = 6379);
    bool Disconnect(); 

    bool Publish(const std::string& channel, const std::string& content); 
    bool Subscribe(const std::string& channel); 
    bool Unsubscribe(const std::string& channel); 

private:
    void Init();

    static void ThreadRoutine(void* arg);
    static void OnConnect(const redisAsyncContext *ac, int status) ;
    static void OnDisconnect(const redisAsyncContext *ac, int status);
    static void OnPublishMsg(redisAsyncContext* ac, void* replyData, void* privData); 
    static void OnSubscribeMsg(redisAsyncContext* ac, void* replyData, void* privData); 

private:
    std::string host;
    uint16_t port;
    redisAsyncContext* asyncContext;

    // libevent
    event_base* evt; 
    // Callback function when receiving data from subscribed channels
    SubscribeCbFunc subCb; 
};

#endif // MiniRedisPubSub_INCLUDED
