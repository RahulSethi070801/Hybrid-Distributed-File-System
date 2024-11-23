#ifndef FILEMETADATA_H
#define FILEMETADATA_H

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <mutex>
#include <unordered_map>

using namespace std;

class FileMetaData {
private:
    string hydfsFileName;
    mutex mtx_fileName;
    int version;

public:
    // Constructor
    FileMetaData();
    // FileMetaData(string hydfsFileName);
    FileMetaData(string hydfsFileName, int version);

    // Getter functions
    string getHydfsFileName() const;
    int getLatestVersion() const;
    int incrementVersion();

    // Setter functions
    void setHydfsFileName(string hydfsFileName);
    void setVersion(int version);

    // assignment operator overloading
    FileMetaData& operator=(const FileMetaData& other);
    // copy constructor
    FileMetaData(const FileMetaData& other);

    std::mutex& getLock();

    // << operator overloading
    friend ostream& operator<<(ostream& os, const FileMetaData& fileMetaData);

};


class FileList {
private:
    unordered_map<string, FileMetaData> filenameToFileMetaData;
    map<int, string> hashToFileName;
    mutex mtx_fileList;

public:
    // Constructor
    FileList();

    // Getter functions
    // void addFile(string fileName);
    void insertFile(string fileName);
    void removeFile(int fileHash);
    void removeFile(string fileName);
    bool isFilePresent(string fileName);
    vector<int> getFilesInRange(int myHash, int nodeHash);
    string getFileName(int hash);
    vector<pair<string, int>> getFileList();
    FileMetaData getFileMetaData(string fileName);
    int getFileVersion(string fileName);
    int incrementFileVersion(string fileName);
    void transferFileToList(string fileName, int version);

    // << operator overloading
    friend ostream& operator<<(ostream& os, FileList& fileList);
    std::mutex& getLockForFile(string file);
};

#endif // FILEMETADATA_H