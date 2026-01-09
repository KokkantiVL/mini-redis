#ifndef KV_STORE_H
#define KV_STORE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>

class KVStore {
public: 
    // Singleton accessor
    static KVStore& instance();

    // General Commands
    bool clearAll();

    // String Operations
    void setString(const std::string& key, const std::string& val);
    bool getString(const std::string& key, std::string& val);
    std::vector<std::string> getAllKeys();
    std::string getKeyType(const std::string& key);
    bool removeKey(const std::string& key);
    bool setExpiry(const std::string& key, int ttlSeconds);
    void cleanupExpired();
    bool renameKey(const std::string& oldKey, const std::string& newKey);

    // List Operations
    std::vector<std::string> getList(const std::string& key);
    ssize_t listSize(const std::string& key);
    void listPushFront(const std::string& key, const std::string& val);
    void listPushBack(const std::string& key, const std::string& val);
    bool listPopFront(const std::string& key, std::string& val);
    bool listPopBack(const std::string& key, std::string& val);
    int listRemove(const std::string& key, int count, const std::string& val);
    bool listGetAt(const std::string& key, int idx, std::string& val);
    bool listSetAt(const std::string& key, int idx, const std::string& val);

    // Hash Operations
    bool hashSet(const std::string& key, const std::string& field, const std::string& val);
    bool hashGet(const std::string& key, const std::string& field, std::string& val);
    bool hashFieldExists(const std::string& key, const std::string& field);
    bool hashDeleteField(const std::string& key, const std::string& field);
    std::unordered_map<std::string, std::string> hashGetAll(const std::string& key);
    std::vector<std::string> hashGetFields(const std::string& key);
    std::vector<std::string> hashGetValues(const std::string& key);
    ssize_t hashSize(const std::string& key);
    bool hashSetMultiple(const std::string& key, const std::vector<std::pair<std::string, std::string>>& pairs);

    // Persistence
    bool saveToDisk(const std::string& filepath);
    bool loadFromDisk(const std::string& filepath);

private:
    KVStore() = default;
    ~KVStore() = default;
    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;

    std::mutex storeMutex_;
    std::unordered_map<std::string, std::string> stringData_;
    std::unordered_map<std::string, std::vector<std::string>> listData_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hashData_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiryTimes_;
};

#endif
