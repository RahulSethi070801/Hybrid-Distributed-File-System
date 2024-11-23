#include "FileMetaData.h"
#include "utils.h"

// Default constructor
FileMetaData::FileMetaData() {}

// Constructor implementation
// FileMetaData::FileMetaData(string hydfsFileName) 
//     : hydfsFileName(hydfsFileName) {}

FileMetaData::FileMetaData(string hydfsFileName, int version) 
    : hydfsFileName(hydfsFileName), version(version) {}

// Copy constructor
FileMetaData::FileMetaData(const FileMetaData& other) 
    : hydfsFileName(other.hydfsFileName), version(other.version) {}

// Getter for hydfsFileName
string FileMetaData::getHydfsFileName() const {
    return hydfsFileName;
}

// Getter for version
int FileMetaData::getLatestVersion() const {
    return version;
}


// Setter for hydfsFileName
void FileMetaData::setHydfsFileName(string hydfsFileName) {
    this->hydfsFileName = hydfsFileName;
}

// Setter for version
void FileMetaData::setVersion(int version) {
    this->version = version;
}

// assignment operator overloading
FileMetaData& FileMetaData::operator=(const FileMetaData& other) {
    if (this != &other) {
        this->hydfsFileName = other.hydfsFileName;
        this->version = other.version;
    }
    return *this;
}

// << operator overloading
ostream& operator<<(ostream& os, const FileMetaData& fileMetaData) {
    os << "FileMetaData: " << fileMetaData.getHydfsFileName() << " : " << fileMetaData.getLatestVersion();
    return os;
}




// Constructor implementation
FileList::FileList() {
    hashToFileName = {};
    filenameToFileMetaData = {};
}

// Add file to the file list
// void FileList::addFile(string fileName) {
//     lock_guard<mutex> lock(mtx_fileList);
//     hashToFileName[hashFunction(fileName)] = fileName;
//     filenameToFileMetaData[fileName] = FileMetaData(fileName);
// }

// Add file to the file list
// void FileList::addFile(string fileName, int version) {
//     lock_guard<mutex> lock(mtx_fileList);
//     if(filenameToFileMetaData.find(fileName) != filenameToFileMetaData.end()){
//         filenameToFileMetaData[fileName].incrementVersion();
//     } else {
//         hashToFileName[hashFunction(fileName)] = fileName;
//         filenameToFileMetaData[fileName] = FileMetaData(fileName, version);
//     }
// }



// << operator overloading
// ostream& operator<<(ostream& os, const FileList& fileList) {
//     for (const auto& pair : fileList.hashToFileName) {
//         os << pair.first << " -> " << pair.second << endl;
//     }
//     return os;
// }

ostream& operator<<(ostream& os, FileList& fileList) {
    for (auto& pair : fileList.hashToFileName) {
        os << pair.first << " -> " << pair.second << " : "<< fileList.getFileVersion(pair.second) << endl;
    }
    // for (const auto& pair : fileList.filenameToFileMetaData) {
    //     os << hashFunction(pair.first) << " -> " << pair.second << endl;
    // }
    return os;
}

// Check if file is present in the file list
bool FileList::isFilePresent(string fileName) {
    cout << "Checking if file is present: " << fileName << endl;
    lock_guard<mutex> lock(mtx_fileList);
    cout<<"acquird lock"<<endl;
    return filenameToFileMetaData.find(fileName) != filenameToFileMetaData.end();
}

// Get files smaller than the given hash
vector<int> FileList::getFilesInRange(int myHash, int nodeHash) {
    cout << "Getting files between " << myHash << " and " << nodeHash << endl;
    lock_guard<mutex> lock(mtx_fileList);
    vector<int> files;
    // consider the the map of file hash as a circular ring find all file hashes which come before the given hash in the ring
    if(nodeHash < myHash){
        for (const auto& pair : hashToFileName) {
            if (pair.first <= nodeHash || pair.first > myHash) {
                files.push_back(pair.first);
            }
        }
    } else {
        for (const auto& pair : hashToFileName) {
            if (pair.first <= nodeHash && pair.first > myHash) {
                files.push_back(pair.first);
            }
        }
    }
    return files;
}

// remove file from the file list
void FileList::removeFile(int fileHash) {
    lock_guard<mutex> lock(mtx_fileList);
    string fileName = hashToFileName[fileHash];
    filenameToFileMetaData.erase(fileName);
    hashToFileName.erase(fileHash);
}

// Get file name from the hash
string FileList::getFileName(int hash) {
    lock_guard<mutex> lock(mtx_fileList);
    return hashToFileName[hash];
}

// Get file name list
vector<pair<string, int>> FileList::getFileList() {
    lock_guard<mutex> lock(mtx_fileList);
    vector<pair<string, int>> fileList;
    for (const auto& pair : filenameToFileMetaData) {
        fileList.push_back({pair.first, pair.second.getLatestVersion()});
    }
    return fileList;
}

// Remove file from the file list
void FileList::removeFile(string fileName) {
    lock_guard<mutex> lock(mtx_fileList);
    int fileHash = hashFunction(fileName);
    filenameToFileMetaData.erase(fileName);
    hashToFileName.erase(fileHash);
}

// Get FileMetaData given file name
FileMetaData FileList::getFileMetaData(string fileName) {
    lock_guard<mutex> lock(mtx_fileList);
    return filenameToFileMetaData[fileName];
}

// get file version
int FileList::getFileVersion(string fileName) {
    lock_guard<mutex> lock(mtx_fileList);
    if(filenameToFileMetaData.find(fileName) == filenameToFileMetaData.end()){
        return 0;
    }
    return filenameToFileMetaData[fileName].getLatestVersion();
}

int FileMetaData::incrementVersion() {
    return ++version;
}

int FileList::incrementFileVersion(string fileName) {
    lock_guard<mutex> lock(mtx_fileList);
    return filenameToFileMetaData[fileName].incrementVersion();
}

std::mutex& FileMetaData::getLock() {
    return mtx_fileName;
}

std::mutex& FileList::getLockForFile(string file) {
    lock_guard<mutex> lock(mtx_fileList);
    return filenameToFileMetaData[file].getLock();
}

// Insert file into the file list
void FileList::insertFile(string fileName) {
    lock_guard<mutex> lock(mtx_fileList);
    hashToFileName[hashFunction(fileName)] = fileName;
    filenameToFileMetaData[fileName] = FileMetaData(fileName, 0);
}

// Transfer file to the file list
void FileList::transferFileToList(string fileName, int version) {
    lock_guard<mutex> lock(mtx_fileList);
    if(filenameToFileMetaData.find(fileName) != filenameToFileMetaData.end()){
        cout<<"ERROR: File already exists"<<endl;
    } else {
        hashToFileName[hashFunction(fileName)] = fileName;
        filenameToFileMetaData[fileName] = FileMetaData(fileName, version);
    }
}