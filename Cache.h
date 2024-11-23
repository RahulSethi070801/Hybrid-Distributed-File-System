#ifndef CACHE_H
#define CACHE_H

#include <unordered_map>
#include <list>
#include <utility>
#include <iostream>

using namespace std;

class Cache {

private:
    size_t cacheSizeLimit;
    std::list<string> cacheOrder;
    std::unordered_map<string, pair<char*, size_t>> cacheMap; 
    mutex mtx_cache;

public:
    Cache(size_t sizeLimit);

    ~Cache();

    void addFileToCache(string &fileName, char *fileContent, size_t contentSize);
    bool getFileFromCache(string &fileName, char *&fileData, size_t &contentSize);
    void printCache();
    void invalidateFileInCache(string &fileName);
    // void updateCache(string fileName, string fileData);

};

#endif // CACHE_H