#include "../include/CommandProcessor.h"
#include "../include/KVStore.h"

#include <vector>
#include <sstream>
#include <algorithm>
#include <exception>
#include <iostream>

/*
 * RESP Protocol Parser
 * Example: *2\r\n$4\r\nPING\r\n$4\r\nTEST\r\n
 * *2 -> array with 2 elements
 * $4 -> next bulk string has 4 characters
 */
std::vector<std::string> parseProtocol(const std::string& input) {
    std::vector<std::string> tokens;
    if (input.empty()) return tokens;

    // Fallback to whitespace splitting if not RESP format
    if (input[0] != '*') { 
        std::istringstream stream(input);
        std::string token;
        while (stream >> token) 
            tokens.push_back(token);
        return tokens;
    }

    size_t pos = 0;
    if (input[pos] != '*') return tokens;
    pos++; // skip '*'

    size_t crlfPos = input.find("\r\n", pos);
    if (crlfPos == std::string::npos) return tokens;

    int elementCount = std::stoi(input.substr(pos, crlfPos - pos));
    pos = crlfPos + 2;

    for (int i = 0; i < elementCount; i++) {
        if (pos >= input.size() || input[pos] != '$') break;
        pos++; // skip '$'

        crlfPos = input.find("\r\n", pos);
        if (crlfPos == std::string::npos) break;
        int strLen = std::stoi(input.substr(pos, crlfPos - pos));
        pos = crlfPos + 2;

        if (pos + strLen > input.size()) break;
        std::string token = input.substr(pos, strLen);
        tokens.push_back(token);
        pos += strLen + 2; // skip token and CRLF
    }
    return tokens;
}

//----------------------
// General Commands
//----------------------
static std::string cmdPing(const std::vector<std::string>& /*args*/, KVStore& /*store*/) {
    return "+PONG\r\n";
}

static std::string cmdEcho(const std::vector<std::string>& args, KVStore& /*store*/) {
    if (args.size() < 2)
        return "-ERR ECHO requires a message\r\n";
    return "+" + args[1] + "\r\n";
}

static std::string cmdFlushAll(const std::vector<std::string>& /*args*/, KVStore& store) {
    store.clearAll();
    return "+OK\r\n";
}

//----------------------
// String Operations
//----------------------
static std::string cmdSet(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3)
        return "-ERR SET requires key and value\r\n";
    store.setString(args[1], args[2]);
    return "+OK\r\n";
}

static std::string cmdGet(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2)
        return "-ERR GET requires key\r\n";
    std::string val;
    if (store.getString(args[1], val))
        return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static std::string cmdKeys(const std::vector<std::string>& /*args*/, KVStore& store) {
    auto allKeys = store.getAllKeys();
    std::ostringstream oss;
    oss << "*" << allKeys.size() << "\r\n";
    for (const auto& k : allKeys)
        oss << "$" << k.size() << "\r\n" << k << "\r\n";
    return oss.str();
}

static std::string cmdType(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2)
        return "-ERR TYPE requires key\r\n";
    return "+" + store.getKeyType(args[1]) + "\r\n";
}

static std::string cmdDel(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2)
        return "-ERR DEL requires key\r\n";
    bool removed = store.removeKey(args[1]);
    return ":" + std::to_string(removed ? 1 : 0) + "\r\n";
}

static std::string cmdExpire(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3)
        return "-ERR EXPIRE requires key and seconds\r\n";
    try {
        int ttl = std::stoi(args[2]);
        if (store.setExpiry(args[1], ttl))
            return "+OK\r\n";
        else
            return "-ERR Key not found\r\n";
    } catch (const std::exception&) {
        return "-ERR Invalid expiration time\r\n";
    }
}

static std::string cmdRename(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3)
        return "-ERR RENAME requires old key and new key\r\n";
    if (store.renameKey(args[1], args[2]))
        return "+OK\r\n";
    return "-ERR Key not found or rename failed\r\n";
}

//----------------------
// List Operations
//----------------------
static std::string cmdLget(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2)
        return "-ERR LGET requires a key\r\n";

    auto items = store.getList(args[1]);
    std::ostringstream oss;
    oss << "*" << items.size() << "\r\n";
    for (const auto& item : items) {
        oss << "$" << item.size() << "\r\n" << item << "\r\n";
    }
    return oss.str();
}

static std::string cmdLlen(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR LLEN requires key\r\n";
    ssize_t len = store.listSize(args[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string cmdLpush(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3) 
        return "-ERR LPUSH requires key and value\r\n";
    for (size_t i = 2; i < args.size(); ++i) {
        store.listPushFront(args[1], args[i]);
    }
    ssize_t len = store.listSize(args[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string cmdRpush(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3) 
        return "-ERR RPUSH requires key and value\r\n";
    for (size_t i = 2; i < args.size(); ++i) {
        store.listPushBack(args[1], args[i]);
    }    
    ssize_t len = store.listSize(args[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string cmdLpop(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR LPOP requires key\r\n";
    std::string val;
    if (store.listPopFront(args[1], val))
        return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static std::string cmdRpop(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR RPOP requires key\r\n";
    std::string val;
    if (store.listPopBack(args[1], val))
        return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static std::string cmdLrem(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 4) 
        return "-ERR LREM requires key, count and value\r\n";
    try {
        int count = std::stoi(args[2]);
        int removed = store.listRemove(args[1], count, args[3]);
        return ":" + std::to_string(removed) + "\r\n";
    } catch (const std::exception&) {
        return "-ERR Invalid count\r\n";
    }
}

static std::string cmdLindex(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3) 
        return "-ERR LINDEX requires key and index\r\n";
    try {
        int idx = std::stoi(args[2]);
        std::string val;
        if (store.listGetAt(args[1], idx, val)) 
            return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
        else 
            return "$-1\r\n";
    } catch (const std::exception&) {
        return "-ERR Invalid index\r\n";
    }
}

static std::string cmdLset(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 4) 
        return "-ERR LSET requires key, index and value\r\n";
    try {
        int idx = std::stoi(args[2]);
        if (store.listSetAt(args[1], idx, args[3]))
            return "+OK\r\n";
        else 
            return "-ERR Index out of range\r\n";
    } catch (const std::exception&) {
        return "-ERR Invalid index\r\n";
    }
}

//----------------------
// Hash Operations
//----------------------
static std::string cmdHset(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 4) 
        return "-ERR HSET requires key, field and value\r\n";
    store.hashSet(args[1], args[2], args[3]);
    return ":1\r\n";
}

static std::string cmdHget(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3) 
        return "-ERR HGET requires key and field\r\n";
    std::string val;
    if (store.hashGet(args[1], args[2], val))
        return "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
    return "$-1\r\n";
}

static std::string cmdHexists(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3) 
        return "-ERR HEXISTS requires key and field\r\n";
    bool exists = store.hashFieldExists(args[1], args[2]);
    return ":" + std::to_string(exists ? 1 : 0) + "\r\n";
}

static std::string cmdHdel(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 3) 
        return "-ERR HDEL requires key and field\r\n";
    bool removed = store.hashDeleteField(args[1], args[2]);
    return ":" + std::to_string(removed ? 1 : 0) + "\r\n";
}

static std::string cmdHgetall(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR HGETALL requires key\r\n";
    auto fields = store.hashGetAll(args[1]);
    std::ostringstream oss;
    oss << "*" << fields.size() * 2 << "\r\n";
    for (const auto& entry : fields) {
        oss << "$" << entry.first.size() << "\r\n" << entry.first << "\r\n";
        oss << "$" << entry.second.size() << "\r\n" << entry.second << "\r\n";
    }
    return oss.str();
}

static std::string cmdHkeys(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR HKEYS requires key\r\n";
    auto fields = store.hashGetFields(args[1]);
    std::ostringstream oss;
    oss << "*" << fields.size() << "\r\n";
    for (const auto& f : fields) {
        oss << "$" << f.size() << "\r\n" << f << "\r\n";
    }
    return oss.str();
}

static std::string cmdHvals(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR HVALS requires key\r\n";
    auto values = store.hashGetValues(args[1]);
    std::ostringstream oss;
    oss << "*" << values.size() << "\r\n";
    for (const auto& v : values) {
        oss << "$" << v.size() << "\r\n" << v << "\r\n";
    }
    return oss.str();
}

static std::string cmdHlen(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 2) 
        return "-ERR HLEN requires key\r\n";
    ssize_t len = store.hashSize(args[1]);
    return ":" + std::to_string(len) + "\r\n";
}

static std::string cmdHmset(const std::vector<std::string>& args, KVStore& store) {
    if (args.size() < 4 || (args.size() % 2) == 1) 
        return "-ERR HMSET requires key followed by field value pairs\r\n";
    std::vector<std::pair<std::string, std::string>> pairs;
    for (size_t i = 2; i < args.size(); i += 2) {
        pairs.emplace_back(args[i], args[i + 1]);
    }
    store.hashSetMultiple(args[1], pairs);
    return "+OK\r\n";
}

CommandProcessor::CommandProcessor() {}

std::string CommandProcessor::execute(const std::string& rawInput) {
    auto args = parseProtocol(rawInput);
    if (args.empty()) return "-ERR Empty command\r\n";

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    KVStore& store = KVStore::instance();

    // General Commands
    if (cmd == "PING")
        return cmdPing(args, store);
    else if (cmd == "ECHO")
        return cmdEcho(args, store);
    else if (cmd == "FLUSHALL")
        return cmdFlushAll(args, store);
    // String Operations
    else if (cmd == "SET")
        return cmdSet(args, store);
    else if (cmd == "GET")
        return cmdGet(args, store);
    else if (cmd == "KEYS")
        return cmdKeys(args, store);
    else if (cmd == "TYPE")
        return cmdType(args, store);
    else if (cmd == "DEL" || cmd == "UNLINK")
        return cmdDel(args, store);
    else if (cmd == "EXPIRE")
        return cmdExpire(args, store);
    else if (cmd == "RENAME")
        return cmdRename(args, store);
    // List Operations
    else if (cmd == "LGET") 
        return cmdLget(args, store);
    else if (cmd == "LLEN") 
        return cmdLlen(args, store);
    else if (cmd == "LPUSH")
        return cmdLpush(args, store);
    else if (cmd == "RPUSH")
        return cmdRpush(args, store);
    else if (cmd == "LPOP")
        return cmdLpop(args, store);
    else if (cmd == "RPOP")
        return cmdRpop(args, store);
    else if (cmd == "LREM")
        return cmdLrem(args, store);
    else if (cmd == "LINDEX")
        return cmdLindex(args, store);
    else if (cmd == "LSET")
        return cmdLset(args, store);
    // Hash Operations
    else if (cmd == "HSET") 
        return cmdHset(args, store);
    else if (cmd == "HGET") 
        return cmdHget(args, store);
    else if (cmd == "HEXISTS") 
        return cmdHexists(args, store);
    else if (cmd == "HDEL") 
        return cmdHdel(args, store);
    else if (cmd == "HGETALL") 
        return cmdHgetall(args, store);
    else if (cmd == "HKEYS") 
        return cmdHkeys(args, store);
    else if (cmd == "HVALS") 
        return cmdHvals(args, store);
    else if (cmd == "HLEN") 
        return cmdHlen(args, store);
    else if (cmd == "HMSET") 
        return cmdHmset(args, store);
    else 
        return "-ERR Unknown command\r\n";
}
