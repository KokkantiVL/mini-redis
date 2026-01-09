#include "../include/KVStore.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

// Singleton accessor
KVStore& KVStore::instance() {
    static KVStore inst;
    return inst;
}

// General Commands
bool KVStore::clearAll() {
    std::lock_guard<std::mutex> guard(storeMutex_);
    stringData_.clear();
    listData_.clear();
    hashData_.clear();
    expiryTimes_.clear();
    return true;
}

// String Operations
void KVStore::setString(const std::string& key, const std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    stringData_[key] = val;
}

bool KVStore::getString(const std::string& key, std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    cleanupExpired();
    auto it = stringData_.find(key);
    if (it != stringData_.end()) {
        val = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> KVStore::getAllKeys() {
    std::lock_guard<std::mutex> guard(storeMutex_);
    cleanupExpired();
    std::vector<std::string> allKeys;
    for (const auto& entry : stringData_) {
        allKeys.push_back(entry.first);
    }
    for (const auto& entry : listData_) {
        allKeys.push_back(entry.first);
    }
    for (const auto& entry : hashData_) {
        allKeys.push_back(entry.first);
    }
    return allKeys;
}

std::string KVStore::getKeyType(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    cleanupExpired();
    if (stringData_.find(key) != stringData_.end()) 
        return "string";
    if (listData_.find(key) != listData_.end())
        return "list";
    if (hashData_.find(key) != hashData_.end()) 
        return "hash";
    return "none";    
}

bool KVStore::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    cleanupExpired();
    bool removed = false;
    removed |= stringData_.erase(key) > 0;
    removed |= listData_.erase(key) > 0;
    removed |= hashData_.erase(key) > 0;
    expiryTimes_.erase(key);
    return removed;
}

bool KVStore::setExpiry(const std::string& key, int ttlSeconds) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    cleanupExpired();
    bool keyExists = (stringData_.find(key) != stringData_.end()) ||
                     (listData_.find(key) != listData_.end()) ||
                     (hashData_.find(key) != hashData_.end());
    if (!keyExists)
        return false;
    
    expiryTimes_[key] = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);
    return true;
}

void KVStore::cleanupExpired() {
    auto currentTime = std::chrono::steady_clock::now();
    for (auto it = expiryTimes_.begin(); it != expiryTimes_.end(); ) {
        if (currentTime > it->second) {
            stringData_.erase(it->first);
            listData_.erase(it->first);
            hashData_.erase(it->first);
            it = expiryTimes_.erase(it);
        } else {
            ++it;
        }
    }
}

bool KVStore::renameKey(const std::string& oldKey, const std::string& newKey) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    cleanupExpired();
    bool found = false;

    auto strIt = stringData_.find(oldKey);
    if (strIt != stringData_.end()) {
        stringData_[newKey] = strIt->second;
        stringData_.erase(strIt);
        found = true;
    }

    auto listIt = listData_.find(oldKey);
    if (listIt != listData_.end()) {
        listData_[newKey] = listIt->second;
        listData_.erase(listIt);
        found = true;
    }

    auto hashIt = hashData_.find(oldKey);
    if (hashIt != hashData_.end()) {
        hashData_[newKey] = hashIt->second;
        hashData_.erase(hashIt);
        found = true;
    }

    auto expIt = expiryTimes_.find(oldKey);
    if (expIt != expiryTimes_.end()) {
        expiryTimes_[newKey] = expIt->second;
        expiryTimes_.erase(expIt);
    }

    return found;
}

// List Operations
std::vector<std::string> KVStore::getList(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = listData_.find(key);
    if (it != listData_.end()) {
        return it->second; 
    }
    return {}; 
}

ssize_t KVStore::listSize(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = listData_.find(key);
    if (it != listData_.end()) 
        return it->second.size();
    return 0;
}

void KVStore::listPushFront(const std::string& key, const std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    listData_[key].insert(listData_[key].begin(), val);
}

void KVStore::listPushBack(const std::string& key, const std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    listData_[key].push_back(val);
}

bool KVStore::listPopFront(const std::string& key, std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = listData_.find(key);
    if (it != listData_.end() && !it->second.empty()) {
        val = it->second.front();
        it->second.erase(it->second.begin());
        return true;
    }
    return false;
}

bool KVStore::listPopBack(const std::string& key, std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = listData_.find(key);
    if (it != listData_.end() && !it->second.empty()) {
        val = it->second.back();
        it->second.pop_back();
        return true;
    }
    return false;
}

int KVStore::listRemove(const std::string& key, int count, const std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    int removedCount = 0;
    auto it = listData_.find(key);
    if (it == listData_.end()) 
        return 0;

    auto& items = it->second;

    if (count == 0) {
        // Remove all occurrences
        auto newEnd = std::remove(items.begin(), items.end(), val);
        removedCount = std::distance(newEnd, items.end());
        items.erase(newEnd, items.end());
    } else if (count > 0) {
        // Remove from head to tail
        for (auto iter = items.begin(); iter != items.end() && removedCount < count; ) {
            if (*iter == val) {
                iter = items.erase(iter);
                ++removedCount;
            } else {
                ++iter;
            }
        }
    } else {
        // Remove from tail to head (count is negative)
        for (auto rit = items.rbegin(); rit != items.rend() && removedCount < (-count); ) {
            if (*rit == val) {
                auto fwdIt = rit.base();
                --fwdIt;
                fwdIt = items.erase(fwdIt);
                ++removedCount;
                rit = std::reverse_iterator<std::vector<std::string>::iterator>(fwdIt);
            } else {
                ++rit;
            }
        }
    }
    return removedCount;
}

bool KVStore::listGetAt(const std::string& key, int idx, std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = listData_.find(key);
    if (it == listData_.end()) 
        return false;

    const auto& items = it->second;
    if (idx < 0)
        idx = items.size() + idx;
    if (idx < 0 || idx >= static_cast<int>(items.size()))
        return false;
    
    val = items[idx];
    return true;
}

bool KVStore::listSetAt(const std::string& key, int idx, const std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = listData_.find(key);
    if (it == listData_.end()) 
        return false;

    auto& items = it->second;
    if (idx < 0)
        idx = items.size() + idx;
    if (idx < 0 || idx >= static_cast<int>(items.size()))
        return false;
    
    items[idx] = val;
    return true;
}

// Hash Operations
bool KVStore::hashSet(const std::string& key, const std::string& field, const std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    hashData_[key][field] = val;
    return true;
}

bool KVStore::hashGet(const std::string& key, const std::string& field, std::string& val) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = hashData_.find(key);
    if (it != hashData_.end()) {
        auto fieldIt = it->second.find(field);
        if (fieldIt != it->second.end()) {
            val = fieldIt->second;
            return true;
        }
    }
    return false;
}

bool KVStore::hashFieldExists(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = hashData_.find(key);
    if (it != hashData_.end())
        return it->second.find(field) != it->second.end();
    return false;
}

bool KVStore::hashDeleteField(const std::string& key, const std::string& field) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = hashData_.find(key);
    if (it != hashData_.end())
        return it->second.erase(field) > 0;
    return false;
}

std::unordered_map<std::string, std::string> KVStore::hashGetAll(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    if (hashData_.find(key) != hashData_.end())
        return hashData_[key];
    return {};
}

std::vector<std::string> KVStore::hashGetFields(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    std::vector<std::string> fields;
    auto it = hashData_.find(key);
    if (it != hashData_.end()) {
        for (const auto& entry : it->second)
            fields.push_back(entry.first);
    }
    return fields;
}

std::vector<std::string> KVStore::hashGetValues(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    std::vector<std::string> values;
    auto it = hashData_.find(key);
    if (it != hashData_.end()) {
        for (const auto& entry : it->second)
            values.push_back(entry.second);
    }
    return values;
}

ssize_t KVStore::hashSize(const std::string& key) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    auto it = hashData_.find(key);
    return (it != hashData_.end()) ? it->second.size() : 0;
}

bool KVStore::hashSetMultiple(const std::string& key, const std::vector<std::pair<std::string, std::string>>& pairs) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    for (const auto& p : pairs) {
        hashData_[key][p.first] = p.second;
    }
    return true;
}

/*
Simple text-based persistence format:
S = String (key-value)
L = List
H = Hash
*/
bool KVStore::saveToDisk(const std::string& filepath) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    std::ofstream outFile(filepath, std::ios::binary);
    if (!outFile) return false;

    for (const auto& entry : stringData_) {
        outFile << "S " << entry.first << " " << entry.second << "\n";
    }
    for (const auto& entry : listData_) {
        outFile << "L " << entry.first;
        for (const auto& item : entry.second)
            outFile << " " << item;
        outFile << "\n";
    }
    for (const auto& entry : hashData_) {
        outFile << "H " << entry.first;
        for (const auto& fieldVal : entry.second) 
            outFile << " " << fieldVal.first << ":" << fieldVal.second;
        outFile << "\n";
    }
    return true;
}

bool KVStore::loadFromDisk(const std::string& filepath) {
    std::lock_guard<std::mutex> guard(storeMutex_);
    std::ifstream inFile(filepath, std::ios::binary);
    if (!inFile) return false;

    stringData_.clear();
    listData_.clear();
    hashData_.clear();

    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        char recordType;
        iss >> recordType;
        if (recordType == 'S') {
            std::string key, val;
            iss >> key >> val;
            stringData_[key] = val;
        } else if (recordType == 'L') {
            std::string key;
            iss >> key;
            std::string item;
            std::vector<std::string> items;
            while (iss >> item)
                items.push_back(item);
            listData_[key] = items;
        } else if (recordType == 'H') {
            std::string key;
            iss >> key;
            std::unordered_map<std::string, std::string> fields;
            std::string pair;
            while (iss >> pair) {
                auto colonPos = pair.find(':');
                if (colonPos != std::string::npos) {
                    std::string field = pair.substr(0, colonPos);
                    std::string val = pair.substr(colonPos + 1);
                    fields[field] = val;
                }
            }
            hashData_[key] = fields;
        }
    }
    return true;
}
