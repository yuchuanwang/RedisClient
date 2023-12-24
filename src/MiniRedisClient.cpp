// Mini C++ client to access Redis

#include <iostream>
#include <sstream>
#include <string.h>
#include <hiredis/hiredis.h>
#include "MiniRedisClient.h"

MiniRedisClient::MiniRedisClient()
{
    Init();
}

MiniRedisClient::~MiniRedisClient()
{
    Clean();
}

void MiniRedisClient::Init()
{
    host = "127.0.0.1";
    port = 6379;
    timeoutSeconds = 3; 
    context = nullptr;
}

void MiniRedisClient::Clean()
{
    if (context)
    {
        redisFree(context);
        context = nullptr;
    }
}

void MiniRedisClient::SetHost(const std::string& host)
{
    this->host = host;
}

std::string MiniRedisClient::GetHost() const
{
    return host;
}

void MiniRedisClient::SetPort(uint16_t port)
{
    this->port = port;
}

uint16_t MiniRedisClient::GetPort() const
{
    return port;
}

void MiniRedisClient::SetTimeoutSeconds(uint32_t sec)
{
    timeoutSeconds = sec; 
}

uint32_t MiniRedisClient::GetTimeoutSeconds() const
{
    return timeoutSeconds;
}

redisContext* MiniRedisClient::GetRawContext()
{
    // The ownership is transferred
    redisContext* ans = context; 
    context = nullptr;
    return ans; 
}

bool MiniRedisClient::Connect()
{
    // Clean if existing
    Clean();

    timeval tv = {timeoutSeconds, 0};
    context = redisConnectWithTimeout(host.c_str(), port, tv);
    if (!context || context->err)
    {
        if (context)
        {
            std::cout << "Failed to connect to Redis server: " << context->errstr << std::endl;
            redisFree(context);
        }
        else
        {
            std::cout << "Can't allocate redis context" << std::endl;
        }

        return false;
    }

    return true;
}

bool MiniRedisClient::Connect(const std::string& host, uint16_t port, uint32_t timeoutSec)
{
    this->host = host;
    this->port = port;
    this->timeoutSeconds = timeoutSec;

    return Connect();
}

// Check the reply type is expected or not
bool MiniRedisClient::CheckReplyType(redisReply* reply, int expectedType) const
{
    if (!reply)
    {
        return false; 
    }

    return (reply->type == expectedType);
}

// Check the reply str is expected or not
bool MiniRedisClient::CheckReplyStr(redisReply* reply, const std::string& expectedStr) const
{
    if (!reply)
    {
        return false; 
    }

    return (expectedStr.compare(reply->str) == 0);
}

// Check the status of reply, and release the reply memory
bool MiniRedisClient::HandleStatusReply(redisReply* reply, std::string& replied) const
{
    bool ret = true; 
    if (!CheckReplyType(reply, REDIS_REPLY_STATUS))
    {
        replied = "";
        ret = false; 
    }
    else
    {
        replied = std::string(reply->str, reply->len);
    }
    freeReplyObject(reply);
    reply = nullptr; 
    return ret;
}

// Return the str of reply, and release the reply memory
bool MiniRedisClient::HandleStringReply(redisReply* reply, std::string& replied) const
{
    bool ret = true; 
    if (!CheckReplyType(reply, REDIS_REPLY_STRING))
    {
        replied = "";
        ret = false; 
    }
    else
    {
        replied = std::string(reply->str, reply->len);
    }
    freeReplyObject(reply);
    reply = nullptr; 
    return ret;
}

// Return the integer of reply, and release the reply memory
bool MiniRedisClient::HandleIntegerReply(redisReply* reply, long long int& replied) const
{
    bool ret = true; 
    if (!CheckReplyType(reply, REDIS_REPLY_INTEGER))
    {
        replied = -1;
        ret = false; 
    }
    else
    {
        replied = reply->integer;
    }
    freeReplyObject(reply);
    reply = nullptr; 
    return ret;
}

bool MiniRedisClient::HandleArrayReply(redisReply* reply, std::vector<std::string>& replied) const
{
    if(!reply)
    {
        return false;
    }

    bool ret = true; 
    if (!CheckReplyType(reply, REDIS_REPLY_ARRAY))
    {
        replied = std::vector<std::string>();
        ret = false; 
    }
    else
    {
        std::size_t count = reply->elements;
        replied.clear();
        for (std::size_t i = 0; i < count; i++)
        {
            std::string val(reply->element[i]->str, reply->element[i]->len);
            replied.push_back(val); 
        }
    }
    freeReplyObject(reply);
    reply = nullptr; 
    return ret;
}

redisReply* MiniRedisClient::execute(const std::string& command, ...) const
{
    va_list args;
    va_start(args, command); 
    redisReply* reply = (redisReply*)redisvCommand(context, command.c_str(), args);
    va_end(args);

    return reply;
}

redisReply* MiniRedisClient::execute(const std::string& command,
    const std::vector<std::string>& args) const
{
    // Number of arguments, including command name
    int argc = args.size() + 1; 
    const char **argv = (const char**)malloc(sizeof(const char**) * argc);
    argv[0] = strdup(command.c_str());
    size_t index = 1;
    for (auto& iter : args)
    {
        argv[index] = strdup(iter.c_str());
        index++;
    }

    // Length of each argument
    size_t *argvLen = (size_t*)malloc(sizeof(size_t) * argc);
    for (int i = 0; i < argc; i++)
    {
        // Use global strlen, instead of Redis strlen
        std::cout << argv[i] << std::endl;
        argvLen[i] = ::strlen(argv[i]);
    }

    redisReply *reply = (redisReply*)redisCommandArgv(context, argc, argv, argvLen);

    // Clean
    for (int i = 0; i < argc; i++)
    {
        free((void*)argv[i]);
    }
    free((void*)argv);
    free((void*)argvLen);

    return reply; 
}

bool MiniRedisClient::append(const std::string& key, const std::string& value, 
    long long int& replied) const
{
    redisReply* reply = execute("APPEND %b %b", 
        key.c_str(), key.size(), 
        value.c_str(), value.size());
    return HandleIntegerReply(reply, replied); 
}

bool MiniRedisClient::auth(const std::string& password, std::string& replied) const
{
    redisReply* reply = execute("AUTH %b", 
        password.c_str(), password.size()); 
    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::client_getname(std::string& replied) const
{
    redisReply* reply = execute("CLIENT GETNAME"); 
    return HandleStringReply(reply, replied);
}

bool MiniRedisClient::client_setname(const std::string& name, std::string& replied) const
{
    redisReply* reply = execute("CLIENT SETNAME %b", 
        name.c_str(), name.size()); 
    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::decr(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("DECR %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::del(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("DEL %b",  
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied); 
}

bool MiniRedisClient::del(const std::vector<std::string>& keys, long long int& replied) const
{
    redisReply* reply = execute("DEL", keys);
    return HandleIntegerReply(reply, replied); 
}

bool MiniRedisClient::exists(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("EXISTS %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied); 
}

bool MiniRedisClient::expire(const std::string& key, uint32_t seconds, 
    long long int& replied) const
{
    redisReply* reply = execute("EXPIRE %b %d", 
        key.c_str(), key.size(), seconds);
    return HandleIntegerReply(reply, replied); 
}

bool MiniRedisClient::get(const std::string& key, std::string& replied) const
{
    redisReply* reply = execute("GET %b", 
        key.c_str(), key.size());
    return HandleStringReply(reply, replied);
}

bool MiniRedisClient::incr(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("INCR %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::keys(const std::string& pattern, std::vector<std::string>& replied) const
{
    redisReply* reply = execute("KEYS %b", 
        pattern.c_str(), pattern.size());
    return HandleArrayReply(reply, replied);
}

std::string MiniRedisClient::ping(const std::string& msg) const
{
    redisReply* reply = nullptr; 
    int expectedType = REDIS_REPLY_STRING;
    if (msg.empty())
    {
        // Empty ping command is specicial
        // It uses REDIS_REPLY_STATUS, and the return value is saved in reply->str
        expectedType = REDIS_REPLY_STATUS; 
        reply = execute("PING");
    }
    else
    {
        reply = execute("PING %b", msg.c_str(), msg.size());
    }

    std::string replied; 
    if (CheckReplyType(reply, expectedType))
    {
        replied = std::string(reply->str, reply->len);
    }
    
    freeReplyObject(reply);
    reply = nullptr; 
    return replied; 
}

bool MiniRedisClient::rename(const std::string& key, const std::string& newKey, 
    std::string& replied) const
{
    redisReply* reply = execute("RENAME %b %b", 
        key.c_str(), key.size(), 
        newKey.c_str(), newKey.size());
    return HandleStatusReply(reply, replied); 
}

bool MiniRedisClient::select(uint32_t dbIndex, std::string& replied) const
{
    redisReply* reply = execute("SELECT %d", dbIndex); 
    //redisReply* reply = (redisReply*)redisCommand(context, "SELECT %d", dbIndex);
    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::set(const std::string& key, const std::string& value, 
    uint32_t ttl, std::string& replied) const
{
    redisReply* reply = nullptr;
    if (ttl > 0)
    {
        reply = execute("SETEX %b %d %b", 
            key.c_str(), key.size(), ttl, 
            value.c_str(), value.size()); 
    }
    else
    {
        reply = execute("SET %b %b", 
            key.c_str(), key.size(), 
            value.c_str(), value.size()); 
    }

    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::set(const std::string& key, long long int value, 
    uint32_t ttl, std::string& replied) const
{
    redisReply* reply = nullptr;
    if (ttl > 0)
    {
        reply = execute("SETEX %b %d %d", 
            key.c_str(), key.size(), ttl, value); 
    }
    else
    {
        reply = execute("SET %b %d", 
            key.c_str(), key.size(), value); 
    }
    
    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::strlen(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("STRLEN %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied); 
}

bool MiniRedisClient::ttl(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("TTL %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::type(const std::string& key, std::string& replied) const
{
    // TODO: Why %b doesn't work? 
    //redisReply* reply = execute("TYPE %b %b", key.c_str(), key.size());
    redisReply* reply = execute("TYPE %s", key.c_str());
    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::hdel(const std::string& key, const std::string& field,
     long long int& replied) const
{
    redisReply* reply = execute("HDEL %b %b", 
        key.c_str(), key.size(), 
        field.c_str(), field.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::hexists(const std::string& key, const std::string& field, 
    long long int& replied)
{
    redisReply* reply = execute("HEXISTS %b %b", 
        key.c_str(), key.size(), 
        field.c_str(), field.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::hget(const std::string& key, const std::string& field, 
    std::string& replied) const
{
    redisReply* reply = execute("HGET %b %b", 
        key.c_str(), key.size(), 
        field.c_str(), field.size());
    return HandleStringReply(reply, replied);
}

bool MiniRedisClient::hgetall(const std::string& key, std::map<std::string, 
    std::string>& replied) const
{
    redisReply* reply = execute("HGETALL %b", 
        key.c_str(), key.size());
    replied.clear();
    std::vector<std::string> items; 
    bool ret = HandleArrayReply(reply, items);
    if (!ret)
    {
        return false;
    }

    // Convert vector to map
    std::size_t sz = items.size(); 
    for (size_t i = 0; i < sz; )
    {
        std::string itemKey = items[i];
        std::string itemVal = items[i+1];
        replied.emplace(std::make_pair(itemKey, itemVal));
        i += 2;
    }
    return ret; 
}

bool MiniRedisClient::hkeys(const std::string& key, std::vector<std::string>& replied) const
{
    redisReply* reply = execute("HKEYS %b", 
        key.c_str(), key.size());
    return HandleArrayReply(reply, replied);
}

bool MiniRedisClient::hlen(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("HLEN %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::hset(const std::string& key, const std::string& field, 
    const std::string& value, long long int& replied) const
{
    redisReply* reply = execute("HSET %b %b %b", 
        key.c_str(), key.size(), 
        field.c_str(), field.size(), 
        value.c_str(), value.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::hvals(const std::string& key, std::vector<std::string>& replied) const
{
    redisReply* reply = execute("HVALS %b", 
        key.c_str(), key.size());
    return HandleArrayReply(reply, replied);
}

bool MiniRedisClient::lindex(const std::string& key, int32_t index, std::string& replied) const
{
    redisReply* reply = execute("LINDEX %b %d", 
        key.c_str(), key.size(), index);
    return HandleStringReply(reply, replied);
}

bool MiniRedisClient::linsert_after(const std::string& key, const std::string& pivot, 
    const std::string& element, long long int& replied) const
{
    redisReply* reply = execute("LINSERT %b AFTER %b %b", 
        key.c_str(), key.size(), 
        pivot.c_str(), pivot.size(), 
        element.c_str(), element.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::linsert_before(const std::string& key, const std::string& pivot, 
    const std::string& element, long long int& replied) const
{
    redisReply* reply = execute("LINSERT %b BEFORE %b %b", 
        key.c_str(), key.size(), 
        pivot.c_str(), pivot.size(), 
        element.c_str(), element.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::llen(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("LLEN %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::lpop(const std::string& key, std::string& replied) const
{
    redisReply* reply = execute("LPOP %b", 
        key.c_str(), key.size());
    return HandleStringReply(reply, replied);
}

bool MiniRedisClient::lpush(const std::string& key, const std::string& element, 
    long long int& replied) const
{
    redisReply* reply = execute("LPUSH %b %b", 
        key.c_str(), key.size(), 
        element.c_str(), element.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::lrem(const std::string& key, int32_t count, 
    const std::string& element, long long int& replied) const
{
    redisReply* reply = execute("LREM %b %d %b", 
        key.c_str(), key.size(), count, 
        element.c_str(), element.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::lset(const std::string& key, int32_t index, 
    const std::string& element, std::string& replied) const
{
    redisReply* reply = execute("LSET %b %d %b", 
        key.c_str(), key.size(), index, 
        element.c_str(), element.size());
    return HandleStatusReply(reply, replied);
}

bool MiniRedisClient::sadd(const std::string& key, const std::string& member, 
    long long int& replied) const
{
    redisReply* reply = execute("SADD %b %b", 
        key.c_str(), key.size(), 
        member.c_str(), member.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::scard(const std::string& key, long long int& replied) const
{
    redisReply* reply = execute("SCARD %b", 
        key.c_str(), key.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::sismember(const std::string& key, const std::string& member, 
    long long int& replied) const
{
    redisReply* reply = execute("SISMEMBER %b %b", 
        key.c_str(), key.size(), 
        member.c_str(), member.size());
    return HandleIntegerReply(reply, replied);
}

bool MiniRedisClient::smembers(const std::string& key, std::vector<std::string>& replied) const
{
    redisReply* reply = execute("SMEMBERS %b", 
        key.c_str(), key.size());
    return HandleArrayReply(reply, replied);
}

bool MiniRedisClient::srem(const std::string& key, const std::string& member, 
    long long int& replied) const
{
    redisReply* reply = execute("SREM %b %b", 
        key.c_str(), key.size(), 
        member.c_str(), member.size());
    return HandleIntegerReply(reply, replied);
}
