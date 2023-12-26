#include <iostream>
#include "MiniRedisClient.h"
#include "MiniRedisPubSub.h"

void TestClient()
{
    std::string ans;
    MiniRedisClient client;
    client.Connect("127.0.0.1", 6379);
    client.client_setname("MiniCppClient", ans);
    client.client_getname(ans); 
    std::cout << ans << std::endl;

    std::cout << client.ping() << std::endl;
    std::cout << client.ping("Hello Redis") << std::endl;

    std::cout << client.auth("password123", ans) << std::endl; 
    std::cout << client.select(1, ans) << std::endl; 
    std::cout << client.select(0, ans) << std::endl; 

    client.set("key 1", "value 1", 3600, ans);
    client.set("key 2", "value 2", 0, ans);

    client.set("key 3", 1234, 3600, ans);
    client.set("key 4", 1001, 0, ans);

    long long int repliedInt = 0;
    client.expire("key 1", 360, repliedInt); 
    client.expire("key 2", 60, repliedInt); 

    client.ttl("key 2", repliedInt);
    client.ttl("key 4", repliedInt);
    client.ttl("invalid", repliedInt);

    client.strlen("key 4", repliedInt);
    client.strlen("invalid", repliedInt);

    client.append("key 4", "2345678", repliedInt);
    client.append("invalid", "ACBDEDF", repliedInt);
    client.strlen("key 4", repliedInt);
    client.strlen("invalid", repliedInt);
    client.del("invalid", repliedInt);

    client.get("key 1", ans);

    client.exists("key 1", repliedInt);
    client.exists("invalid", repliedInt);

    std::string keyDel = "key 2";
    client.del(keyDel, repliedInt);
    keyDel = "key 3";
    client.del(keyDel, repliedInt);
    keyDel = "key 4";
    client.del(keyDel, repliedInt);
    keyDel = "key 5";
    client.del(keyDel, repliedInt);

    std::string repliedStr;
    client.hset("domains", "example", "example.com", repliedInt); 
    client.hset("domains", "abc", "abc.com", repliedInt); 
    client.hget("domains", "example", repliedStr);

    client.hset("newHash", "me", "1234567890", repliedInt); 
    client.hget("newHash", "you", repliedStr);
    client.hdel("newHash", "me", repliedInt);

    std::map<std::string, std::string> repliedMap; 
    client.hgetall("domains", repliedMap);
    std::vector<std::string> repliedArray; 
    client.hkeys("domains", repliedArray);
    client.hvals("domains", repliedArray);

    client.hexists("domains", "abc", repliedInt);
    client.hexists("domains", "invalid", repliedInt);
    client.hlen("domains", repliedInt);

    client.keys("*", repliedArray);
    client.keys("user*", repliedArray);

    client.rename("users", "friends", repliedStr);
    client.rename("friends", "users", repliedStr);

    client.type("domains", repliedStr);
    client.type("users", repliedStr);
    client.type("username", repliedStr);
    client.type("Null", repliedStr);

    client.decr("counter", repliedInt); 
    client.incr("counter", repliedInt); 

    client.lpush("List123", "item 1", repliedInt);
    client.lpush("List123", "item 2", repliedInt);
    client.lpush("List123", "item 3", repliedInt);
    client.lpush("List123", "item 4", repliedInt);
    client.llen("List123", repliedInt);
    client.lpop("List123", repliedStr); 
    client.llen("List123", repliedInt);
    client.linsert_before("List123", "item 3", "item 2+", repliedInt);
    client.linsert_after("List123", "item 3", "item 3+", repliedInt);
    client.lset("List123", 2, "item set 2", repliedStr); 
    client.lrem("List123", 0, "item 2+", repliedInt);
    client.lindex("List123", 0, repliedStr);
    client.lindex("List123", -1, repliedStr);
    //client.del("List123", repliedInt);

    client.sadd("set123", "ele 1", repliedInt);
    client.sadd("set123", "ele 2", repliedInt);
    client.sadd("set123", "ele 3", repliedInt);
    client.sadd("set123", "ele 4", repliedInt);
    client.sadd("set123", "ele 5", repliedInt);
    client.sadd("set123", "ele 5", repliedInt);
    client.sadd("set123", "ele 4", repliedInt);
    client.scard("set123", repliedInt);
    client.sismember("set123", "ele 1", repliedInt);
    client.sismember("set123", "ele 8", repliedInt);
    client.smembers("set123", repliedArray);
    client.srem("set123", "ele 1", repliedInt);
    client.srem("set123", "ele 8", repliedInt);
    //client.del("set123", repliedInt);

    client.set("Blank space", "value", 0, repliedStr);
    std::vector<std::string> keysDel;
    keysDel.push_back("List123");
    keysDel.push_back("set123");
    keysDel.push_back("Blank space");
    client.del(keysDel, repliedInt);

    std::cout << repliedInt << std::endl; 
}

void SubscribeCb(const std::string& channel, const std::string& content)
{
    std::cout << "Subscriber CB receives channel: " << channel << ", data: " << content << std::endl; 
}

void TestSub()
{
    MiniRedisPubSub sub;
    sub.SetSubscribeCb(SubscribeCb);
    sub.Connect("127.0.0.1", 6379);
    sub.Subscribe("testChannel1");
    sub.Subscribe("testChannel2");

    int interval = 60;
    int i = 0;
    while (i < interval)
    {
        sleep(1);
        i++; 
    }
    
    std::cout << "Subscribing done" << std::endl; 
}

void TestPub()
{
    MiniRedisPubSub pub;
    pub.Connect("127.0.0.1", 6379);

    int interval = 60;
    int i = 0;
    std::string content("Content from C++ client"); 
    while (i < interval)
    {
        pub.Publish("channelFromCpp", content); 
        sleep(1);
        i++; 
    }

    std::cout << "Publishing done" << std::endl; 
}

int main()
{
    TestClient();
    //TestPub();
    TestSub();
}
