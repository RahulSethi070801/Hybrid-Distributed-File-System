#include "Cache.h"
#include <unordered_map>
#include <list>
#include <utility>
#include <iostream>

using namespace std;

Cache::Cache(size_t sizeLimit) : cacheSizeLimit(sizeLimit) {}

Cache::~Cache() {
    for (auto it = cacheMap.begin(); it != cacheMap.end(); it++) {
        delete[] it->second.first;
    }
}

void Cache::addFileToCache(string &fileName, char *fileContent, size_t contentSize) {
    lock_guard<mutex> lock(mtx_cache);

    if (cacheMap.size() >= cacheSizeLimit) {
        string lastFile = cacheOrder.back();
        cacheOrder.pop_back();
        delete[] cacheMap[lastFile].first;
        cacheMap.erase(lastFile);
    }
    char *fileData = new char[contentSize];
    memcpy(fileData, fileContent, contentSize);
    cacheMap[fileName] = make_pair(fileData, contentSize);
    cacheOrder.push_front(fileName);
}

bool Cache::getFileFromCache(string &fileName, char *&fileContent, size_t &contentSize) {
    lock_guard<mutex> lock(mtx_cache);
    if (cacheMap.find(fileName) == cacheMap.end()) {
        return false;
    }
    cacheOrder.remove(fileName);
    cacheOrder.push_front(fileName);
    fileContent = new char[cacheMap[fileName].second];
    memcpy(fileContent, cacheMap[fileName].first, cacheMap[fileName].second);
    contentSize = cacheMap[fileName].second;
    return true;
}

void Cache::invalidateFileInCache(string &fileName) {
    lock_guard<mutex> lock(mtx_cache);
    if (cacheMap.find(fileName) != cacheMap.end()) {
        delete[] cacheMap[fileName].first;
        cacheMap.erase(fileName);
        // cacheOrder.remove(fileName);
        cacheOrder.remove_if([&fileName](const string &item) { return item == fileName; });
        cout<<"File invalidated in cache: "<<fileName<<endl;
        printCache();
    }
}

void Cache::printCache() {
    cout<<"Cache order: ";
    for (auto it = cacheOrder.begin(); it != cacheOrder.end(); it++) {
        cout << *it << " ";
    }
    cout << endl;
    cout<<"Cache map: ";
    for (auto it = cacheMap.begin(); it != cacheMap.end(); it++) {
        cout << it->first << " ";
    }
    cout << endl;
}