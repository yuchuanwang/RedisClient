// Mini C++ client to access Redis
// Depends on hiredis: 
// sudo apt install -y libhiredis-dev
// which installs 0.1x version.
// 
// If need new version, please build from source https://github.com/redis/hiredis/
// git clone https://github.com/redis/hiredis
// cd hiredis
// make
// sudo make install
// sudo ldconfig /usr/local/lib
//

#ifndef MiniRedisClient_INCLUDED
#define MiniRedisClient_INCLUDED

#include <string>
#include <vector>
#include <map>

struct redisContext;
struct redisReply;

class MiniRedisClient
{
public:
    MiniRedisClient();
    ~MiniRedisClient();

    // Getter and Setter of this instance
    void SetHost(const std::string& host);
    std::string GetHost() const;
    void SetPort(uint16_t port);
    uint16_t GetPort() const;
    void SetTimeoutSeconds(uint32_t sec);
    uint32_t GetTimeoutSeconds() const;

    // Return the raw redisContext pointer to user, and transfer the ownership
    // The user should release the pointer
    redisContext* GetRawContext();

    // Connect to Redis Server
    bool Connect();
    bool Connect(const std::string& host, uint16_t port = 6379, uint32_t timeoutSec = 3);

    //////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////
    // Check the reply type is expected or not
    bool CheckReplyType(redisReply* reply, int expectedType) const;
    // Check the reply str is expected or not
    bool CheckReplyStr(redisReply* reply, const std::string& expectedStr) const;

    // Check the status of reply, and release the reply memory
    bool HandleStatusReply(redisReply* reply, std::string& replied) const; 
    // Return the str of reply, and release the memory
    bool HandleStringReply(redisReply* reply, std::string& replied) const; 
    // Return the integer of reply, and release the memory
    bool HandleIntegerReply(redisReply* reply, long long int& replied) const; 
    // Return the array of reply, and release the memory
    bool HandleArrayReply(redisReply* reply, std::vector<std::string>& replied) const; 
    //////////////////////////////////////////////////

    //////////////////////////////////////////////////
    // Redis commands
    //////////////////////////////////////////////////
    // https://redis.io/commands/append/
    // If key already exists and is a string, this command appends the value at the end of the string
    // If key does not exist it is created and set as an empty string
    // Integer reply: the length of the string after the append operation
    bool append(const std::string& key, const std::string& value, long long int& replied) const;

    // https://redis.io/commands/auth/
    // Authenticates the current connection
    // Redis versions prior of 6 were only able to understand the password
    // Simple string reply: OK, or an error if the password, or username/password pair, is invalid
    bool auth(const std::string& password, std::string& replied) const; 

    // Get client name from Redis server
    bool client_getname(std::string& replied) const;
    // Set client name to Redis server
    // Useful when running: redis-cli client list
    // Cannot contain blank spaces, newlines or special chars
    bool client_setname(const std::string& name, std::string& replied) const;

    // https://redis.io/commands/decr/
    // Decrements the number stored at key by one
    // Integer reply: the value of the key after decrementing it
    bool decr(const std::string& key, long long int& replied) const;
    
    // https://redis.io/commands/del/
    // Removes the specified key. A key is ignored if it does not exist.
    // Reply the number of keys that were removed
    bool del(const std::string& key, long long int& replied) const; 
    bool del(const std::vector<std::string>& keys, long long int& replied) const;

    // https://redis.io/commands/exists/
    // Returns if key exists
    // Integer reply: the number of key that exist
    bool exists(const std::string& key, long long int& replied) const;  
    
    // https://redis.io/commands/expire/
    // Set a timeout on key
    // Integer reply: 0 if the timeout was not set; 
    // for example, the key doesn't exist, 
    // or the operation was skipped because of the provided arguments
    // Integer reply: 1 if the timeout was set
    bool expire(const std::string& key, uint32_t seconds, long long int& replied) const; 

    // https://redis.io/commands/get/
    // Get the value of key.
    // GET only handles string values.
    bool get(const std::string& key, std::string& replied) const;

    // https://redis.io/commands/incr/
    // Increments the number stored at key by one
    // Integer reply: the value of the key after the increment
    bool incr(const std::string& key, long long int& replied) const;

    // https://redis.io/commands/keys/
    // Returns all keys matching pattern
    // Using * to get all keys
    // Array reply: a list of keys matching pattern
    bool keys(const std::string& pattern, std::vector<std::string>& replied) const; 

    // https://redis.io/commands/ping/
    // Ping-Pong to Redis server
    // Returns PONG if no argument is provided, otherwise return a copy of the argument as a bulk
    // If just sending: PING, the reply type is REDIS_REPLY_STATUS
    // If sending: PING XXX, the reply type is REDIS_REPLY_STRING
    std::string ping(const std::string& msg = "") const;

    // https://redis.io/commands/rename/
    // Renames key to newkey
    // Simple string reply: OK
    bool rename(const std::string& key, const std::string& newKey, std::string& replied) const;  

    // https://redis.io/commands/select/
    // Select the logical database having the specified zero-based numeric index
    // By default, the index is from 0 to 15
    // New connections always use the database 0
    // Simple string reply: OK
    bool select(uint32_t dbIndex, std::string& replied) const; 

    // String and generic related commands
    // https://redis.io/commands/set/
    // Set key to hold the string value
    // If key already holds a value, it is overwritten, regardless of its type
    // ttl = 0 means no limit
    bool set(const std::string& key, const std::string& value, 
        uint32_t ttl, std::string& replied) const;
    bool set(const std::string& key, long long int value, 
        uint32_t ttl, std::string& replied) const;

    // https://redis.io/commands/strlen/
    // Returns the length of the string value stored at key
    // Integer reply: the length of the string stored at key, or 0 when the key does not exist
    bool strlen(const std::string& key, long long int& replied) const;  

    // https://redis.io/commands/ttl/
    // Returns the remaining time to live of a key that has a timeout
    // Integer reply: TTL in seconds
    // Integer reply: -1 if the key exists but has no associated expiration
    // Integer reply: -2 if the key does not exist
    bool ttl(const std::string& key, long long int& replied) const;

    // https://redis.io/commands/type/
    // Returns the string representation of the type of the value stored at key
    // string, list, set, zset, hash and stream
    // Simple string reply: the type of key, or none when key doesn't exist
    // FIXME: Actually it returns with Status type, not string type
    bool type(const std::string& key, std::string& replied) const; 

    // Hash related commands
    // https://redis.io/commands/hdel/
    // Removes the specified field from the hash stored at key
    // Integer reply: the number of fields that were removed from the hash,
    // excluding any specified but non-existing fields
    bool hdel(const std::string& key, const std::string& field, long long int& replied) const;
    
    // https://redis.io/commands/hexists/
    // Returns if field is an existing field in the hash stored at key
    // Integer reply: 0 if the hash does not contain the field, or the key does not exist
    // Integer reply: 1 if the hash contains the field
    bool hexists(const std::string& key, const std::string& field, long long int& replied);

    // https://redis.io/commands/hget/
    // Returns the value associated with field in the hash stored at key
    // Bulk string reply: The value associated with the field
    // FIXME: the document says it returns bulk string, while I get normal string
    // Nil reply: If the field is not present in the hash or key does not exist
    bool hget(const std::string& key, const std::string& field, std::string& replied) const;

    // https://redis.io/commands/hgetall/
    // Returns all fields and values of the hash stored at key
    // In the original returned value, every field name is followed by its value, 
    // so the length of the reply is twice the size of the hash
    // I change the returned value to std::map instead, so each item is key:value
    // Array reply: a list of fields and their values stored in the hash, 
    // or an empty list when key does not exist
    bool hgetall(const std::string& key, std::map<std::string, std::string>& replied) const; 

    // https://redis.io/commands/hkeys/
    // Returns all field names in the hash stored at key
    // Array reply: a list of fields in the hash, or an empty list when the key does not exist
    bool hkeys(const std::string& key, std::vector<std::string>& replied) const; 
    
    // https://redis.io/commands/hlen/
    // Returns the number of fields contained in the hash stored at key
    // Integer reply: the number of fields in the hash, or 0 when the key does not exist
    bool hlen(const std::string& key, long long int& replied) const;

    // https://redis.io/commands/hset/
    // Sets the specified fields to their respective values in the hash stored at key
    // Overwrites the values of specified fields that exist in the hash
    // If key doesn't exist, a new key holding a hash is created
    // Integer reply: the number of fields that were added
    bool hset(const std::string& key, const std::string& field, 
        const std::string& value, long long int& replied) const;

    // https://redis.io/commands/hvals/
    // Returns all values in the hash stored at key
    // Array reply: a list of values in the hash, or an empty list when the key does not exist
    bool hvals(const std::string& key, std::vector<std::string>& replied) const;

    // List related commands
    // https://redis.io/commands/lindex/
    // Returns the element at index in the list stored at key
    // 0 means the first element
    // -1 means the last element
    // Nil reply: when index is out of range.
    // Bulk string reply: the requested element.
    bool lindex(const std::string& key, int32_t index, std::string& replied) const; 

    // https://redis.io/commands/linsert/
    // Inserts element in the list stored at key either before or after the reference value pivot
    // Integer reply: the list length after a successful insert operation.
    // Integer reply: 0 when the key doesn't exist.
    // Integer reply: -1 when the pivot wasn't found
    bool linsert_after(const std::string& key, const std::string& pivot, 
        const std::string& element, long long int& replied) const;
    bool linsert_before(const std::string& key, const std::string& pivot, 
        const std::string& element, long long int& replied) const;

    // https://redis.io/commands/llen/
    // Returns the length of the list stored at key
    // Integer reply: the length of the list
    bool llen(const std::string& key, long long int& replied) const; 
    
    // https://redis.io/commands/lpop/
    // Removes and returns the first elements of the list stored at key
    // Nil reply: if the key does not exist
    // Bulk string reply: when called without the count argument, the value of the first element
    // Array reply: when called with the count argument, 
    // a list of popped elements - Unsupported in this function
    bool lpop(const std::string& key, std::string& replied) const; 

    // https://redis.io/commands/lpush/
    // Insert element at the head of the list stored at key
    // Integer reply: the length of the list after the push operation
    bool lpush(const std::string& key, const std::string& element, long long int& replied) const; 

    // https://redis.io/commands/lrem/
    // Removes the first count occurrences of elements equal to element from the list stored at key. 
    // The count argument influences the operation in the following ways:
    // count > 0: Remove elements equal to element moving from head to tail.
    // count < 0: Remove elements equal to element moving from tail to head.
    // count = 0: Remove all elements equal to element.
    // Integer reply: the number of removed elements
    bool lrem(const std::string& key, int32_t count, const std::string& element, 
        long long int& replied) const; 

    // https://redis.io/commands/lset/
    // Sets the list element at index to element
    // Simple string reply: OK
    bool lset(const std::string& key, int32_t index, 
        const std::string& element, std::string& replied) const; 

    // Set related commands
    // https://redis.io/commands/sadd/
    // Add the specified member to the set stored at key
    // Integer reply: the number of elements that were added to the set, 
    // not including all the elements already present in the set
    bool sadd(const std::string& key, const std::string& member, long long int& replied) const;

    // https://redis.io/commands/scard/
    // Returns the number of elements of the set stored at key
    // Integer reply: the number of elements of the set, or 0 if the key does not exist
    bool scard(const std::string& key, long long int& replied) const;

    // https://redis.io/commands/sismember/
    // Returns if member is a member of the set stored at key
    // Integer reply: 0 if the element is not a member of the set, or when the key does not exist
    // Integer reply: 1 if the element is a member of the set
    bool sismember(const std::string& key, const std::string& member, long long int& replied) const;

    // https://redis.io/commands/smembers/
    // Returns all the members of the set value stored at key
    // Array reply: all members of the set
    bool smembers(const std::string& key, std::vector<std::string>& replied) const;

    // https://redis.io/commands/srem/
    // Remove the specified member from the set stored at key
    // Integer reply: the number of members that were removed from the set, 
    // not including non existing members
    bool srem(const std::string& key, const std::string& member, long long int& replied) const;

    // Sorted Set related commands
    // ... ...

    // https://redis.io/docs/manual/pipelining/
    // Use pipeline to improve performance by batch operation
    bool pipeline(const std::vector<std::string>& commands, std::vector<std::string>& replied) const;

    // Raw command interface
    // https://redis.io/commands/
    // Thera are so many commands...
    // For those not wrapped, please use this interface 
    // User should parse and free the redisReply by freeReplyObject()
    redisReply* execute(const std::string& command, ...) const;
    // Execute command with list of arguments
    redisReply* execute(const std::string& command, const std::vector<std::string>& args) const;
    //////////////////////////////////////////////////

private:
    void Init();
    void Clean();

private:
    std::string host;
    uint16_t port;
    // Timeout when connecting
    uint32_t timeoutSeconds; 
    redisContext* context;
};

#endif // MiniRedisClient_INCLUDED
