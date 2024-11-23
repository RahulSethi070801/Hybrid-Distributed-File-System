#include "HyDFS.h"
#include "HyDFSMessage.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <dirent.h>
#include <unordered_set>
#include <sstream>
using namespace std;

#define FILES_DIRECTORY "./files/"
#define HYDFS_FILES_DIRECTORY "/hydfs"
#define LOCAL_FILES_DIRECTORY "/local"
#define REPLICATION_FACTOR 2
#define MULTI_APPEND_LOCAL_FILE "appendFile"
#define MERGE_INTERVAL 5

string getLocalFilesPath(string filesDirectory, string filename){
    return filesDirectory + LOCAL_FILES_DIRECTORY + "/" + filename;
}

void deleteFile(string filepath){
    if (remove(filepath.c_str()) != 0) {
        std::cerr << "Error deleting file: " << strerror(errno) << std::endl;
    } else {
        std::cout << getCurrentFullTimestamp()<<" : " << "File deleted: " << filepath << std::endl;
    }
}

string getHyDFSFilesPath(string filesDirectory, string filename, int version){
    return filesDirectory + HYDFS_FILES_DIRECTORY + "/" + filename + '$' + to_string(version);
}

bool fileExists(string localFilepath){
    struct stat buffer;
    return (stat(localFilepath.c_str(), &buffer) == 0);
}

bool copyFile(string source, string dest){
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(dest, std::ios::binary);
    if (!src) {
        std::cerr << "Error copying file: " << strerror(errno) << std::endl;
        return false;
    }
    dst << src.rdbuf();
    if (!src || !dst) {
        std::cerr << "Error moving file to primary directory: " << strerror(errno) << std::endl;
        return false;
    } else {
        std::cout << getCurrentFullTimestamp()<<" : " << "File moved to primary directory: " << dest << std::endl;
        return true;
    }
}

HyDFS::HyDFS(char *port) : cache(4000000) {
    tcpPort = port;
    std::cout << getCurrentFullTimestamp()<<" : " << "HyDFS started on port: " << tcpPort << std::endl;
    bytesReceived = 0;
    bytesSent = 0;
    totalBytesReceived= 0;
    totalBytesSent = 0;
}

HyDFS::~HyDFS() {
    // std::string dirPath = filesDirectory;
    
    // if (rmdir(dirPath.c_str()) == -1) {
    //     std::cerr << "Error deleting directory: " << strerror(errno) << std::endl;
    // } else {
    //     std::cout << getCurrentFullTimestamp()<<" : " << "Directory deleted: " << dirPath << std::endl;
    // }
}

void HyDFS::getFileHandler(HyDFSMessage message, int clientSockfd){
   
    // cout << getCurrentFullTimestamp()<<" : "<<"Get file handler invoked\n";
    string fileName = message.getFileName();
    int numberOfShards = primaryFilsList.getFileVersion(fileName);
    if(numberOfShards == 0){
        cout<<"Getting from replica\n";
        numberOfShards = secondaryFilesList.getFileVersion(fileName);
    }
    if(numberOfShards == 0){
        cerr<<"File does not exist"<<endl;
        return;
    }
    for (int shard = 1; shard <= numberOfShards; shard++) {
        string filePath = getHyDFSFilesPath(filesDirectory, fileName, shard);
        sendFileToSocket(filePath, clientSockfd);
    }

}

void HyDFS::putFileHelper(string fileName, int version, int clientSockfd){
    std::lock_guard<std::mutex> lock(primaryFilsList.getLockForFile(fileName));
    HyDFSMessage response = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::READY_TO_RECEIVE_FILE, fileName);
    sendMessageToSocket(response, clientSockfd);
    string hydfsFilepath = getHyDFSFilesPath(filesDirectory, fileName, version);

    bool didReceive = receiveFileFromSocket(hydfsFilepath, clientSockfd, fileName, false);
    if(didReceive){
        // add file to primary files list
        
        primaryFilsList.incrementFileVersion(fileName);
        cout << getCurrentFullTimestamp()<<" : "<<"New version of file "<<fileName<<" is "<<primaryFilsList.getFileVersion(fileName)<<endl;
    }
}


void HyDFS::putFileHandler(HyDFSMessage msg, int clientSockfd){
    cout << getCurrentFullTimestamp()<<" : "<<"Put file handler invoked\n";
    string fileName = msg.getFileName();
    int version = msg.getVersion();
    
    cout << getCurrentFullTimestamp()<<" : "<<"File name "<<fileName<<" version "<<version<<endl;
    if(!primaryFilsList.isFilePresent(fileName)){
        if(version == 1){
            cout << getCurrentFullTimestamp()<<" : "<<"inserted file"<<endl;
            primaryFilsList.insertFile(fileName);
            putFileHelper(fileName, version, clientSockfd);
        } else {
            cerr<<"Invalid version"<<endl;
            HyDFSMessage response = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::FILE_DOES_NOT_EXIST, fileName);
            sendMessageToSocket(response, clientSockfd);
            return;
        }
    } else {
        int myVersion = primaryFilsList.getFileVersion(fileName);
        if(version <= myVersion){
            cerr<<"Invalid version"<<endl;
            HyDFSMessage response = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::FILE_ALREADY_EXISTS, fileName);
            sendMessageToSocket(response, clientSockfd);
            return;
        } else {
            putFileHelper(fileName, version, clientSockfd);
        }
    }       
}

void HyDFS::appendFileHandler(HyDFSMessage message, int clientSockfd){
    cout << getCurrentFullTimestamp()<<" : "<<"Append file handler invoked\n";
    string fileName = message.getFileName();
    int version = primaryFilsList.getFileVersion(fileName);
    cout << getCurrentFullTimestamp()<<" : "<<"File name "<<fileName<<" version "<<version<<endl;
    if(!primaryFilsList.isFilePresent(fileName)){
        cerr<<"File does not exist"<<endl;
        cout << getCurrentFullTimestamp()<<" : "<<"File does not exist "<<primaryFilsList.getFileVersion(fileName)<<endl;
        HyDFSMessage response = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::FILE_DOES_NOT_EXIST, fileName);
        sendMessageToSocket(response, clientSockfd);
        return;
    }

    std::lock_guard<std::mutex> lock(primaryFilsList.getLockForFile(fileName));

    string shardPath = getHyDFSFilesPath(filesDirectory, fileName, primaryFilsList.getFileVersion(fileName)+1);
    HyDFSMessage response = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::READY_TO_RECEIVE_FILE, fileName);
    sendMessageToSocket(response, clientSockfd);
    bool didReceive = receiveFileFromSocket(shardPath, clientSockfd, fileName, false);
    if(didReceive){
        cout << getCurrentFullTimestamp()<<" : "<<"File received successfully"<<endl;
        primaryFilsList.incrementFileVersion(fileName);
    }
}

void HyDFS::getPrimaryFileListHandler(HyDFSMessage message, int clientSockfd){
    // cout << getCurrentFullTimestamp()<<" : "<<"Get file list handler invoked\n";
    vector<pair<string, int>> filesList = primaryFilsList.getFileList();
    if(filesList.size() == 0){
        // cout << getCurrentFullTimestamp()<<" : "<<"No files in primary files list\n";
    }
    // cout << getCurrentFullTimestamp()<<" : "<<"Size of file list: "<<filesList.size()<<endl;
    HyDFSMessage response = HyDFSMessage::getFileListMessage(HyDFSMessage::HyDFSMessageType::GET_FILE_LIST_RESPONSE, filesList);
    sendMessageToSocket(response, clientSockfd);
}

void HyDFS::getShardHandler(HyDFSMessage message, int clientSockfd){
    // cout << getCurrentFullTimestamp()<<" : "<<"Get shard handler invoked\n";
    string fileName = message.getFileName();
    int version = message.getVersion();
    int myVersion = primaryFilsList.getFileVersion(fileName);
    if(version > myVersion){
        cerr<<"Invalid version"<<endl;
        HyDFSMessage response = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::FILE_DOES_NOT_EXIST, fileName);
        sendMessageToSocket(response, clientSockfd);
        return;
    }
    string filePath = getHyDFSFilesPath(filesDirectory, fileName, version);
    sendFileToSocket(filePath, clientSockfd);
}

void HyDFS::multiAppendHandler(HyDFSMessage message){
    appendFile(MULTI_APPEND_LOCAL_FILE, message.getFileName());
}

void HyDFS::getFileAllListHandler(HyDFSMessage message, int clientSockfd){
    
    vector<pair<string, int>> filesList = primaryFilsList.getFileList();
    vector<pair<string, int>> secFileList = secondaryFilesList.getFileList();
    filesList.insert(filesList.end(), secFileList.begin(), secFileList.end());
    if(filesList.size() == 0){
        // cout << getCurrentFullTimestamp()<<" : "<<"No files in primary files list\n";
    }
    HyDFSMessage response = HyDFSMessage::getFileListMessage(HyDFSMessage::HyDFSMessageType::GET_FILE_LIST_RESPONSE, filesList);
    sendMessageToSocket(response, clientSockfd);
}

void HyDFS::invalidateCacheHandler(HyDFSMessage message){
    string fileName = message.getFileName();
    cout<<"Invalidating cache for file: "<<fileName<<endl;
    cache.invalidateFileInCache(fileName);
}

void HyDFS::handleClient(int clientSockfd) {
    // std::cout << getCurrentFullTimestamp()<<" : " << "Handling client" << std::endl;
    HyDFSMessage message = receiveMessageFromSocket(clientSockfd);

    switch(message.getType()) {
        case HyDFSMessage::HyDFSMessageType::GET_FILE:
            std::cout << getCurrentFullTimestamp()<<" : " << "GET_FILE" << std::endl;
            getFileHandler(message, clientSockfd);
            break;
        case HyDFSMessage::HyDFSMessageType::PUT_FILE:
            std::cout << getCurrentFullTimestamp()<<" : " << "PUT_FILE" << std::endl;
            putFileHandler(message, clientSockfd);
            break;
        case HyDFSMessage::HyDFSMessageType::GET_FILE_LIST:
            // std::cout << getCurrentFullTimestamp()<<" : " << "GET_FILE_LIST" << std::endl;
            getPrimaryFileListHandler(message, clientSockfd);
            break;
        case HyDFSMessage::HyDFSMessageType::APPEND_FILE:
            std::cout << getCurrentFullTimestamp()<<" : " << "APPEND_FILE" << std::endl;
            appendFileHandler(message, clientSockfd);
            break;
        case HyDFSMessage::HyDFSMessageType::GET_SHARD:
            // std::cout << getCurrentFullTimestamp()<<" : " << "GET_SHARD" << std::endl;
            getShardHandler(message, clientSockfd);
            break;
        case HyDFSMessage::HyDFSMessageType::MULTI_APPEND:
            std::cout << getCurrentFullTimestamp()<<" : " << "MULTI_APPEND" << std::endl;
            multiAppendHandler(message);
            break;
        case HyDFSMessage::HyDFSMessageType::INVALIDATE_CACHE:
            // std::cout << "INVALIDATE_CACHE" << std::endl;
            invalidateCacheHandler(message);
            break;
        case HyDFSMessage::HyDFSMessageType::MERGE_FILE:
            std::cout << getCurrentFullTimestamp()<<" : " << "MERGE_FILE" << std::endl;
            mergeFileHandler(message);
            break;
        case HyDFSMessage::HyDFSMessageType::GET_ALL_FILE_LIST:
            std::cout << getCurrentFullTimestamp()<<" : " << "GET_ALL_FILE_LIST" << std::endl;
            getFileAllListHandler(message, clientSockfd);
            break;
        default:
            std::cout << getCurrentFullTimestamp()<<" : " << "Invalid hydfs message type received - " << HyDFSMessage::messageTypeToString(message.getType()) << std::endl;
    }
    close(clientSockfd);
}

void HyDFS::mergeFileHandler(HyDFSMessage message){
   
    cout<< getCurrentFullTimestamp()<<" : "<<"Merging file - "<<message.getFileName()<<endl;
    int fileHash = hashFunction(message.getFileName());
    Node primaryNode = ring.getPrimaryNodeForFile(fileHash);
    // cout<<"Primary node: "<<primaryNode.getNodeId()<<endl;
    vector<int> replicaNodes = ring.getKSuccessorsOf(REPLICATION_FACTOR, primaryNode.getRingHash());
    // cout<<"Replica nodes: "<<replicaNodes.size()<<endl;
    for(int hash: replicaNodes){
        Node replicaNode = ring.getNode(hash);
        // cout<<"Replica node: "<<replicaNode.getNodeId()<<endl;
        if(replicaNode.getNodeId() == selfNodeId){
            // cout<<"I am a replica for file"<<message.getFileName()<<endl;
            HyDFSMessage getListMessage = HyDFSMessage::getInformationalMessage(HyDFSMessage::HyDFSMessageType::GET_FILE_LIST);
            int sockfd = sendMessageToNode(getListMessage, primaryNode);
            if(sockfd == -1){
                cerr<<"Error sending message"<<endl;
                return;
            }
            HyDFSMessage response = receiveMessageFromSocket(sockfd);
            vector<pair<string, int>> filesList = response.getFilesList();
            // cout<<"Files list size: "<<filesList.size()<<endl;
            for(auto file : filesList){
                cout<<file.first<<" "<<file.second<<endl;
                if(file.first == message.getFileName()){
                    mergeFile(file.first, file.second, primaryNode);
                    break;
                }
            }
            // cout<<"FIle not found"<<endl;
            break;
        }
    }
        

    
}

void HyDFS::membershipUpdate(std::vector<Node>& nodes) {
    for(Node node : nodes) {
        updateRing(node);
    }
}

void HyDFS::updateRing(Node node) {
    if(node.getStatus() == Status::alive) {
        ring.addNode(node);
    } else if (node.getStatus() == Status::failed || node.getStatus() == Status::left) {
        ring.removeNode(node);
    }
    
}


void HyDFS::listMyFiles() {
    cout << getCurrentFullTimestamp()<<" : "<<"File list for "<< ring.getSelfNode().getRingHash()<<" :"<<endl;
    cout << primaryFilsList;
    cout << secondaryFilesList;

}

void HyDFS::runCommand(std::string command) {
    if(command == "c"){
        cout << getCurrentFullTimestamp()<<" : \n"<<ring;
    } else if(command == "store"){
        listMyFiles();
    }
    // else if the command start with create
    else if (command.find("create") == 0) {
        string hydfsFileName, localFileName;
        stringstream ss(command);
        string word;
        int i = 0;
        while(ss >> word){
            if(i == 1){
                localFileName = word;
            } else if(i == 2){
                hydfsFileName = word;
            }
            i++;
        }
        
        cout << getCurrentFullTimestamp()<<" : "<<localFileName<<" "<<hydfsFileName<<endl;
        createFile(localFileName, hydfsFileName);
    } else if (command.find("getfromreplica") == 0){
        string vmId, hydfsFileName, localFileName;
        stringstream ss(command);
        string word;
        int i = 0;
        while(ss >> word){
            if(i == 1){
                vmId = word;
            } else if(i == 2){
                hydfsFileName = word;
            } else if(i == 3){
                localFileName = word;
            }
            i++;
        }
        getFileFromReplica(vmId, hydfsFileName, localFileName);
    }
    else if (command.find("get") == 0) {
        string hydfsFileName, localFileName;
        stringstream ss(command);
        string word;
        int i = 0;
        while(ss >> word){
            if(i == 1){
                hydfsFileName = word;
            } else if(i == 2){
                localFileName = word;
            }
            i++;
        }
        requestFile(hydfsFileName, localFileName);
    } 
    
    else if (command.find("append") == 0) {
        string hydfsFileName, localFileName;
        stringstream ss(command);
        string word;
        int i=0;
        while (ss >> word) {
            if (i == 1) {
                localFileName = word;
            } else if (i == 2) {
                hydfsFileName = word;
            }
            i++;
        }
        appendFile(localFileName, hydfsFileName);
    }
    else if (command == "rls") {
        listMyFiles();
    }
    else if (command == "cache") {
        cache.printCache();
    }
    else if (command == "ping") {
        std::cout << getCurrentFullTimestamp()<<" : " << "Pinging" << std::endl;
        mergeAllReplicas();
    } else if (command == "push"){
        monitorFileLists();
    } else if (command.find("multiappend") == 0) {
        stringstream ss(command);
        string word, hydfsFileName;
        int numOfAppends = 0, numOfClients = 0;
        int i = 0;
        while (ss >> word) {
            if (i == 1) {
                hydfsFileName = word;
            } else if (i == 2) {
                try {
                    numOfAppends = stoi(word);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid argument: " << e.what() << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Out of range: " << e.what() << std::endl;
                }
            } else if (i == 3) {
                try {
                    numOfClients = stoi(word);
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid argument: " << e.what() << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Out of range: " << e.what() << std::endl;
                }
            }
            i++;
        }
        multiAppend(hydfsFileName, numOfAppends, numOfClients);
    }
    else if (command == "list_mem_ids"){
        cout << getCurrentFullTimestamp()<<" : \n"<<ring;
    } else if (command.find("ls") == 0){
        stringstream ss(command);
        string word = "", hydfFilename;
        int i = 0;
        while(ss >> word){
            if(i == 1){
                hydfFilename = word;
            }
            i++;
        }
        listNodesHavingFile(hydfFilename);
    } else if (command.find("merge") == 0){
        stringstream ss(command);
        string word = "", hydfsFileName;
        int i = 0;
        while(ss >> word){
            if(i == 1){
                hydfsFileName = word;
            }
            i++;
        }
        mergeFile(hydfsFileName);
    } else {
        std::cout << getCurrentFullTimestamp()<<" : " << "Invalid command" << std::endl;
    }
}

void HyDFS::getFileFromReplica(string vmId, string hydfsFileName, string localFileName){
    cout<<"Getting file "<< hydfsFileName <<"from replica "<< vmId<< " to "<<localFileName<<endl;
    vector<int> nodes = ring.getListOfNodes();
    for(int nodeHash : nodes){
        Node node = ring.getNode(nodeHash);
        if(node.getNodeId() == vmId){
            cout << getCurrentFullTimestamp()<<" : "<<"Getting file from replica "<<node.getNodeId()<<endl;
            HyDFSMessage message = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::GET_FILE, hydfsFileName);
            int sockfd = sendMessageToNode(message, node);
            if(sockfd == -1){
                cerr<<"Error sending message"<<endl;
                return;
            }
            string localFilePath = getLocalFilesPath(filesDirectory, localFileName);
            receiveFileFromSocket(localFilePath, sockfd, hydfsFileName, false);
            return;
            
        }
    }
    cout << getCurrentFullTimestamp()<<" : "<<"No replica found"<<endl;
}

void HyDFS::mergeFile(string fileName) {
    vector<int> nodes = ring.getListOfNodes();
    HyDFSMessage message = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::MERGE_FILE, fileName);
    for(int nodeId : nodes){
        Node node = ring.getNode(nodeId);
        if(node.getNodeId() == selfNodeId){
            mergeFileHandler(message);
        } else {
            int sockfd = sendMessageToNode(message, node);
            if(sockfd == -1){
                cerr<<"Error sending message"<<endl;
                return;
            }
            close(sockfd);
        }
        
    }
}

unordered_set<string> HyDFS::getAllFileListFromNode(Node node){
    vector<pair<string, int>> filesList;
    if(node.getNodeId() == selfNodeId){
        filesList = primaryFilsList.getFileList();
        vector<pair<string, int>> secList = secondaryFilesList.getFileList();
        filesList.insert(filesList.end(), secList.begin(), secList.end());
    } else {
        HyDFSMessage message = HyDFSMessage::getInformationalMessage(HyDFSMessage::HyDFSMessageType::GET_ALL_FILE_LIST);
        int sockfd = sendMessageToNode(message, node);
        if(sockfd == -1){
            cerr<<"Error sending message"<<endl;
            return {};
        }
        HyDFSMessage response = receiveMessageFromSocket(sockfd);
        filesList = response.getFilesList();
        
    }
    unordered_set<string> files;
    for(auto file : filesList){
        files.insert(file.first);
    }
    return files;
}

void HyDFS::listNodesHavingFile(string fileName){
    int fileHash = hashFunction(fileName);
    Node primaryNode = ring.getPrimaryNodeForFile(fileHash);
    cout << getCurrentFullTimestamp()<<" : "<<"Nodes for file "<<fileName<<", hash = "<<fileHash<<endl;
    unordered_set<string> files = getAllFileListFromNode(primaryNode);
    if(files.find(fileName) == files.end()){
        cout<<"File not present in HyDFS"<<endl;
        return;
    }
    vector<int> nodes = ring.getKSuccessorsOf(REPLICATION_FACTOR, primaryNode.getRingHash());
    cout << getCurrentFullTimestamp()<<" : "<<primaryNode.getRingHash()<<"->"<<primaryNode.getNodeId()<<endl;
    for(int nodeId : nodes){
        Node node = ring.getNode(nodeId);
        unordered_set<string> files = getAllFileListFromNode(node);
        if(files.find(fileName) != files.end()){
            cout << getCurrentFullTimestamp()<<" : "<<node.getRingHash()<<"->"<<node.getNodeId()<<endl;
        }
    }
}

void HyDFS::multiAppend(string hydfsFileName, int numOfAppends, int numOfClients){

    sendInvalidateCacheMessage(hydfsFileName);

    vector<int> nodes = ring.getListOfNodes();
    // shrink nodes to number of clients
    while(nodes.size() > numOfClients){
        nodes.pop_back();
    }
    for(int i = 0; i < numOfAppends; i++){
        int nodeIndex = i % nodes.size();
        Node node = ring.getNode(nodes[nodeIndex]);

        if(node.getNodeId() == selfNodeId){
            appendFile(MULTI_APPEND_LOCAL_FILE, hydfsFileName);
        } else {
            HyDFSMessage message = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::MULTI_APPEND, hydfsFileName);
            int sockfd = sendMessageToNode(message, node);
            if(sockfd == -1){
                cerr<<"Error sending message"<<endl;
                return;
            }
            close(sockfd);
        }
        
    }
}

void createDirectorty(string dirPath){
    if (mkdir(dirPath.c_str(), 0777) == -1) {
        std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
    } else {
        std::cout << getCurrentFullTimestamp()<<" : " << "Directory created: " << dirPath << std::endl;
    }
}

void HyDFS::setSelfNode(Node node) {
    selfNodeId = node.getNodeId();
    ring.addNode(node);
    cout << getCurrentFullTimestamp()<<" : "<<"Self node id: "<<selfNodeId<<endl;
}




void HyDFS::tcpListener(int tcpSockfd) {
    std::vector<std::thread> clientThreads;

    if (listen(tcpSockfd, 10) < 0) {
        std::cerr << "Listen failed.\n";
        close(tcpSockfd);
        return;
    }

    while(threadsRunning) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientSockfd = accept(tcpSockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if(clientSockfd < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            continue;
        }
        // std::cout << getCurrentFullTimestamp()<<" : " << "Connection accepted" << std::endl;

        std::thread clientThread(&HyDFS::handleClient, this, clientSockfd);
        clientThreads.emplace_back(std::move(clientThread));
    }

    for (std::thread &t : clientThreads) {
        t.join();
    }


}

void HyDFS::requestFile(string hydfsFileName, string localFileName){

    auto start = std::chrono::high_resolution_clock::now();

    string localFilePath = getLocalFilesPath(filesDirectory, localFileName);
    // string primaryFilePath = getHyDFSFilesPath(filesDirectory, hydfsFileName);

    char *fileData = nullptr;
    size_t contentSize = 0;
    bool inCache = cache.getFileFromCache(hydfsFileName, fileData, contentSize);
    cout<<"File in cache: "<<inCache<<endl;
    if (inCache) {
        std::ofstream localFile(localFilePath, std::ios::binary | std::ios::app);
        if (localFile.is_open()) {
            localFile.write(fileData, contentSize);
            localFile.close();
            delete[] fileData;
            std::cout << "File retrieved from cache: " << localFilePath << std::endl;
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            std::cout << "Time taken to retrieve file: " << elapsed.count() << "s" << std::endl;
            return;
        } else {
            std::cerr << "Error opening local file: " << localFilePath << std::endl;
        }
    }

    int fileHash = hashFunction(hydfsFileName);
    cout << getCurrentFullTimestamp()<<" : "<<"hash of file: "<<hydfsFileName<<" is "<<fileHash<<endl;

    Node primaryNode = ring.getPrimaryNodeForFile(fileHash);

    if(selfNodeId == primaryNode.getNodeId()){
        int maxVersion = primaryFilsList.getFileMetaData(hydfsFileName).getLatestVersion();
        cout << getCurrentFullTimestamp()<<" : "<<"Mapped to self node"<<endl;
        std::ofstream dst(localFilePath, std::ios::binary | std::ios::trunc);
        vector<char> buffer;
        for (int version = 1; version <= maxVersion; version++) {
            string chunkFilePath = getHyDFSFilesPath(filesDirectory, hydfsFileName, version);
            // cout << getCurrentFullTimestamp()<<" : "<<chunkFilePath<<endl;
            std::ifstream src(chunkFilePath, std::ios::binary);
            dst << src.rdbuf();
            buffer.insert(buffer.end(), std::istreambuf_iterator<char>(src), std::istreambuf_iterator<char>());
        }
        dst.close();
        // cache.addFileToCache(hydfsFileName, buffer.data(), buffer.size());
        dst.close();
        // copyFile(primaryFilePath, localFilePath);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Time taken to retrieve file: " << elapsed.count() << "s" << std::endl;
        return;
    }

    cout << getCurrentFullTimestamp()<<" : "<<"Requesting file from the primary node for file: "<<hydfsFileName<<" is "<<primaryNode.getNodeId()<<endl;
    receiveFileFromNode(localFilePath, primaryNode, hydfsFileName);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken to retrieve file: " << elapsed.count() << "s" << std::endl;
}

void HyDFS::receiveFileFromNode(string filepath, Node node, string hydfsFilename){
    HyDFSMessage message = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::GET_FILE, hydfsFilename);
    int sockfd = sendMessageToNode(message, node);
    if(sockfd == -1){
        cerr<<"Error sending message"<<endl;
        return;
    }
    receiveFileFromSocket(filepath, sockfd, hydfsFilename, true);
    close(sockfd);
}

bool HyDFS::receiveFileFromSocket(string filepath, int sockfd, string fileName, bool forCache) {
    std::ofstream
    file(filepath, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error creating file: " << strerror(errno) << std::endl;
        close(sockfd);
        return false;
    }

    vector<char> cachebuffer;
    char buffer[1024] = {0};
    int bytesRead = 0;
    while ((bytesRead = recv(sockfd, buffer, sizeof(buffer), 0)) > 0) {
        file.write(buffer, bytesRead);
        if (forCache) {
            cachebuffer.insert(cachebuffer.end(), buffer, buffer + bytesRead);
        }
    }
    if (forCache)
        cache.addFileToCache(fileName, cachebuffer.data(), cachebuffer.size());
    file.close();
    close(sockfd);
    std::cout << getCurrentFullTimestamp()<<" : " << "File received: " << filepath << std::endl;
    return true;
}

void HyDFS::createFile(string localFilename, string hydfsFilename){
    string localFilepath = getLocalFilesPath(filesDirectory, localFilename);

    int hash = hashFunction(hydfsFilename);
    Node targetNode = ring.getPrimaryNodeForFile(hash);
    int version = 1;

    // check if file is present in local file path
    if(!fileExists(localFilepath)){
        std::cerr << "Error: local File does not exist: " << localFilename << std::endl;
        return;
    }

    if(selfNodeId == targetNode.getNodeId()){
        if(primaryFilsList.isFilePresent(hydfsFilename)){
            std::cerr << "Error: File already exists: " << hydfsFilename << std::endl;
            return;
        }
        primaryFilsList.insertFile(hydfsFilename);
        std::lock_guard<std::mutex> lock(primaryFilsList.getLockForFile(hydfsFilename));
        int newVersion = version;
        std::string hydfsFilePath = getHyDFSFilesPath(filesDirectory, hydfsFilename, newVersion);
        bool success = copyFile(localFilepath, hydfsFilePath);
        if(success){
            primaryFilsList.incrementFileVersion(hydfsFilename);
        }
        return;
    }

    // string hydfsFileShard = hydfsFilename + "$" + to_string(version);

    cout << getCurrentFullTimestamp()<<" : "<<"Primary node for file: "<<hydfsFilename<<" is "<<targetNode.getNodeId()<<endl;
    sendFileToNode(localFilepath, targetNode, hydfsFilename, version);
    return;
}

void HyDFS::sendFileToNode(string filepath, Node node, string hydfsFilename, int version) {
    
    HyDFSMessage message = HyDFSMessage::getFileNameVersionMessage(HyDFSMessage::HyDFSMessageType::PUT_FILE, hydfsFilename, version);
    // cout << getCurrentFullTimestamp()<<" : "<<"Sending message to node: "<<node.getNodeId()<<endl;
    int sockfd = sendMessageToNode(message, node);
    if(sockfd == -1){
        cerr<<"Error sending message"<<endl;
        return;
    }

    HyDFSMessage responseMessage = receiveMessageFromSocket(sockfd);

    if (responseMessage.getType() == HyDFSMessage::HyDFSMessageType::READY_TO_RECEIVE_FILE) {
        cout << getCurrentFullTimestamp()<<" : "<<"sending the files"<<endl;
        sendFileToSocket(filepath, sockfd);
    } else {
        cerr<<"Error sending file to node - "<<responseMessage.getTypeString()<<endl;
    }

    close(sockfd);
}

HyDFSMessage HyDFS::receiveMessageFromSocket(int sockfd){
    char buffer[1024] = {0};
    int bytesRead = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead < 0) {
        std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
        return HyDFSMessage::getInformationalMessage(HyDFSMessage::HyDFSMessageType::ERROR_RECEIVING_MESSAGE);
    }
    totalBytesReceived += bytesRead;
    bytesReceived += bytesRead;
    buffer[bytesRead] = '\0';
    std::string bufferStr(buffer);
    HyDFSMessage message = HyDFSMessage::deserializeHyDFSMessage(bufferStr);
    // cout << getCurrentFullTimestamp()<<" : "<<"Received message "<< message.getTypeString() <<" from sockfd: "<<sockfd<<endl;
    return message;
}

void HyDFS::sendFileToSocket(string filepath, int sockfd){
    FILE *file = fopen(filepath.c_str(), "rb");
    if (file == NULL) {
        std::cerr << "Error opening file: " << strerror(errno) << std::endl;
        return;
    }

    char buffer[1024] = {0};
    int bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(sockfd, buffer, bytesRead, 0) < 0) {
            std::cerr << "Error sending file: " << strerror(errno) << std::endl;
            break;
        }
        totalBytesSent += bytesRead;
        bytesSent += bytesRead;
    }
    fclose(file);
}

int HyDFS::sendMessageToNode(HyDFSMessage message, Node node){
    // cout << getCurrentFullTimestamp()<<" : "<<"Sending message "<< message.getTypeString() <<" to node: "<<node.getNodeId()<<endl;
    int sockfd = getSocketForNode(node);
    if(sockfd == -1){
        cerr<<"Error getting socket for node"<<endl;
        return -1;
    }
    sendMessageToSocket(message, sockfd);
    return sockfd;
}

int HyDFS::getSocketForNode(Node node){
    struct sockaddr_in nodeAddr = getTCPAddrFromNode(node);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&nodeAddr, sizeof(nodeAddr)) < 0) {
        std::cerr << "Error connecting to node: " << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void HyDFS::sendMessageToSocket(HyDFSMessage message, int sockfd){
    // cout << getCurrentFullTimestamp()<<" : "<<"Sending message "<< message.getTypeString() <<" to sockfd: "<<sockfd<<endl;
    string serializedMessage = message.serializeMessage();
    int bytes = 0;
    if ((bytes = send(sockfd, serializedMessage.c_str(), serializedMessage.size(), 0)) < 0) {
        std::cerr << "Error sending message: " << strerror(errno) << std::endl;
    }
    totalBytesSent += bytes;
    bytesSent += bytes;
}

void HyDFS::sendInvalidateCacheMessage(string hydfsFileName){
    cache.invalidateFileInCache(hydfsFileName);
    vector<int> nodes = ring.getListOfNodes();
    for (int nodeId : nodes) {
        Node node = ring.getNode(nodeId);
        if (node.getNodeId() != selfNodeId) {
            cout<<"Sending cache invalidation message to node: "<<node.getNodeId()<<endl;
            HyDFSMessage message = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::INVALIDATE_CACHE, hydfsFileName);
            int sockfd = sendMessageToNode(message, node);
            if (sockfd == -1) {
                cerr << "Error sending cache invalidation message to node " << node.getNodeId() << endl;
                continue;
            }
            close(sockfd);
        }
    }
}

void HyDFS::appendFile(string localFileName, string hydfsFileName){

    string localFilePath = getLocalFilesPath(filesDirectory, localFileName);
    // cout << getCurrentFullTimestamp()<<" : " << "checking if file exists: " << localFilePath << endl;
    if(!fileExists(localFilePath)){
        std::cerr << "Error: local File does not exist: " << localFileName << std::endl;
        return;
    }

    sendInvalidateCacheMessage(hydfsFileName);

    int fileHash = hashFunction(hydfsFileName);

    Node primaryNode = ring.getPrimaryNodeForFile(fileHash);

    if(selfNodeId == primaryNode.getNodeId()){
        cout << getCurrentFullTimestamp()<<" : "<<"Mapped to self node"<<endl;
        if (!primaryFilsList.isFilePresent(hydfsFileName)) {
            std::cerr <<  "File " << hydfsFileName << " is not present in primary node" << std::endl;
            return;
        }
        
        // lock file
        std::lock_guard<std::mutex> lock(primaryFilsList.getLockForFile(hydfsFileName));
        int currentVersion = primaryFilsList.getFileVersion(hydfsFileName);
        string versionedFilePath = getHyDFSFilesPath(filesDirectory, hydfsFileName, currentVersion+1);
        bool success = copyFile(localFilePath, versionedFilePath);
        if(success){
            primaryFilsList.incrementFileVersion(hydfsFileName);
        }

        // unlock file

        return;
    } else {
        appendFileToNode(localFilePath, primaryNode, hydfsFileName);
    }

    
}

void HyDFS::appendFileToNode(string filepath, Node node, string hydfsFileName) {
    HyDFSMessage message = HyDFSMessage::getFileNameMessage(HyDFSMessage::HyDFSMessageType::APPEND_FILE, hydfsFileName);
    int sockfd = sendMessageToNode(message, node);
    if(sockfd == -1){
        cerr<<"Error sending message"<<endl;
        return;
    }

    HyDFSMessage response = receiveMessageFromSocket(sockfd);
    if(response.getType() == HyDFSMessage::HyDFSMessageType::READY_TO_RECEIVE_FILE){
        sendFileToSocket(filepath, sockfd);
    } else {
        cerr<<"Error sending file to node - "<<response.getTypeString()<<endl;
    }

    close(sockfd);
}


void HyDFS::start() {
    startEpoch = getCurrentTSinEpoch();
    threadsRunning = true;
    cout << getCurrentFullTimestamp()<<" : "<<"Starting HyDFS tcp listener"<<endl;
    tcpSocket = setupTCPSocket(tcpPort);
    std::thread listenerThread(&HyDFS::tcpListener, this, tcpSocket);
    threads.emplace_back(std::move(listenerThread));
    std::thread predecessorPingerThread(&HyDFS::predecessorPinger, this);
    threads.emplace_back(std::move(predecessorPingerThread));
    std::thread monitorFileListsThread(&HyDFS::monitorFileLists, this);
    threads.emplace_back(std::move(monitorFileListsThread));
}

void HyDFS::printBandwidth(){
    cout << getCurrentFullTimestamp()<<" : "<<"Total bytes sent: "<<totalBytesSent<<endl;
    cout << getCurrentFullTimestamp()<<" : "<<"Total bytes received: "<<totalBytesReceived<<endl;
    double diff = differenceWithCurrentEpoch(startEpoch);
    cout << getCurrentFullTimestamp()<<" : "<<"Upload bandwidth: "<<(double)totalBytesSent*1000/(diff*1024*1024)<<endl;
    cout << getCurrentFullTimestamp()<<" : "<<"Download bandwidth: "<<(double)totalBytesReceived*1000/(diff*1024*1024)<<endl;
}


void HyDFS::setLocalFilesPath(string path) {
    if(path == ""){
        std::string dirPath = FILES_DIRECTORY + selfNodeId;
        createDirectorty(dirPath);
        // create three subdirectories primary, replica, local
        std::string hydfsDirPath = dirPath + HYDFS_FILES_DIRECTORY;
        createDirectorty(hydfsDirPath);

        std::string localDirPath = dirPath + LOCAL_FILES_DIRECTORY;
        createDirectorty(localDirPath);

        filesDirectory = dirPath;
    } else {
        filesDirectory = path;
    }
}

void HyDFS::stopThreads() {
    threadsRunning = false;
    for (std::thread &t : threads) {
        t.join();
    }
}

void HyDFS::mergeFile(string fileName, int numberOfShards, Node fromNode){
    string file = fileName;
    int version = numberOfShards;
    int myVersion = secondaryFilesList.getFileVersion(file);
    if(version > myVersion){
        cout << getCurrentFullTimestamp()<<" : "<<"Merging file "<<file<<" from node "<<fromNode.getNodeId()<<endl;
        string start = getCurrentTSinEpoch();
        bytesReceived = 0;
        bytesSent = 0;
        if(myVersion == 0){
            secondaryFilesList.insertFile(file);
        }
        std::lock_guard<std::mutex> lock(secondaryFilesList.getLockForFile(file));
        for(int i = myVersion + 1; i <= version; i++){
            // cout << getCurrentFullTimestamp()<<" : "<<"Requesting shard "<<i<<" of file "<<file<<" out of "<<version<<endl;
            HyDFSMessage message = HyDFSMessage::getFileNameVersionMessage(HyDFSMessage::HyDFSMessageType::GET_SHARD, file, i);
            int sockfd = sendMessageToNode(message, fromNode);
            if(sockfd == -1){
                cerr<<"Error sending message"<<endl;
                return;
            }
            bool didReceive = receiveFileFromSocket(getHyDFSFilesPath(filesDirectory, file, i), sockfd, file, false);
            if(didReceive){
                secondaryFilesList.incrementFileVersion(file);
            } else {
                cout << getCurrentFullTimestamp()<<" : "<<"Error receiving file list from predecessor: "<<fromNode.getNodeId()<<endl;
            }
        }
        double diff = differenceWithCurrentEpoch(start);
        cout << getCurrentFullTimestamp()<<" : "<<"Time taken to merge file "<<file<<" from node "<<fromNode.getNodeId()<<" is "<<diff/1000<<endl;
        cout << getCurrentFullTimestamp()<<" : "<<"Completed merge "<<file<<" from node "<<fromNode.getNodeId()<<endl;
        printCurrentBandwidth(diff);
        // printBandwidth();
    } 
}

void HyDFS::printCurrentBandwidth(double diff){
    cout << getCurrentFullTimestamp()<<" : "<<"Bytes sent: "<<bytesSent<<endl;
    cout << getCurrentFullTimestamp()<<" : "<<"Bytes received: "<<bytesReceived<<endl;
    cout << getCurrentFullTimestamp()<<" : "<<"curr upload bandwidth: "<<(double)bytesSent*1000/(diff*1024*1024)<<endl;
    cout << getCurrentFullTimestamp()<<" : "<<"curr download bandwidth: "<<(double)bytesReceived*1000/(diff*1024*1024)<<endl;
}

void HyDFS::mergeAllReplicas(){
    vector<int> predecessors = ring.getKPredecessors(REPLICATION_FACTOR);

        for(int predecessor : predecessors){
            Node node = ring.getNode(predecessor);
            HyDFSMessage message = HyDFSMessage::getInformationalMessage(HyDFSMessage::HyDFSMessageType::GET_FILE_LIST);
            int clientSockfd = sendMessageToNode(message, node);
            if(clientSockfd == -1){
                cerr<<"Error sending message"<<endl;
                return;
            }
            HyDFSMessage response = receiveMessageFromSocket(clientSockfd);
            if(response.getType() == HyDFSMessage::HyDFSMessageType::GET_FILE_LIST_RESPONSE){
                // cout << getCurrentFullTimestamp()<<" : "<<"Received file list from predecessor: "<<node.getNodeId()<<endl;
                vector<pair<string, int>> filesList = response.getFilesList();
                // cout << getCurrentFullTimestamp()<<" : "<<"Size of file list: "<<filesList.size()<<endl;
                for(auto fileVersion : filesList){
                    mergeFile(fileVersion.first, fileVersion.second, node);
                }
            } else {
                cout << getCurrentFullTimestamp()<<" : "<<"Error receiving file list from predecessor: "<<node.getNodeId()<<endl;
            }
        }
}

void HyDFS::predecessorPinger() {
    while(threadsRunning) {
        sleep(MERGE_INTERVAL);
        mergeAllReplicas();
    }
}



void HyDFS::monitorFileLists(){
    while(threadsRunning){
        sleep(MERGE_INTERVAL);

        vector<pair<string, int>> primaryFiles = primaryFilsList.getFileList();
        vector<pair<string, int>> secondaryFiles = secondaryFilesList.getFileList();
        vector<int> predecessors = ring.getKPredecessors(REPLICATION_FACTOR);
        unordered_set<int> predecessorSet(predecessors.begin(), predecessors.end());

        for(auto& fileVersion : primaryFiles){
            string fileName = fileVersion.first;
            int fileHash = hashFunction(fileName);
            Node targetNode = ring.getPrimaryNodeForFile(fileHash);

            if(targetNode.getRingHash() == ring.getSelfNode().getRingHash()){
                continue;
            } else {
                bool moveToReplica = predecessorSet.find(targetNode.getRingHash()) != predecessorSet.end();
                transferFileToNode(fileName, targetNode, moveToReplica);
            }
        }

        for(auto& fileVersion : secondaryFiles){
            string fileName = fileVersion.first;
            int fileHash = hashFunction(fileName);
            Node targetNode = ring.getPrimaryNodeForFile(fileHash);

            if(targetNode.getRingHash() == ring.getSelfNode().getRingHash()){
                // TODO: transfer file
                secondaryFilesList.removeFile(fileName);
                primaryFilsList.transferFileToList(fileName, fileVersion.second);
            } else if (predecessorSet.find(targetNode.getRingHash()) == predecessorSet.end()){
                cout << getCurrentFullTimestamp()<<" : "<<"DELETING FIEL FROM SECONDARY"<<endl;
                // TODO: delete all shards of the file
                // lock file
                std::lock_guard<std::mutex> lock(secondaryFilesList.getLockForFile(fileName));
                for(int i=1;i<=secondaryFilesList.getFileVersion(fileName);i++){
                    deleteFile(getHyDFSFilesPath(filesDirectory, fileName, i));
                }
                // deleteFile(getHyDFSFilesPath(filesDirectory, fileName));
                secondaryFilesList.removeFile(fileName);
            }
        }
    }
}

void HyDFS::transferFileToNode(string fileName, Node node, bool moveToReplica){  
    
    int myVersions = primaryFilsList.getFileVersion(fileName);
    primaryFilsList.removeFile(fileName);
    for(int i=1;i<=myVersions;i++){
        string filePath = getHyDFSFilesPath(filesDirectory, fileName, i);
        HyDFSMessage message = HyDFSMessage::getFileNameVersionMessage(HyDFSMessage::HyDFSMessageType::PUT_FILE, fileName, i);
        int socket = sendMessageToNode(message, node);
        if(socket == -1){
            cerr<<"Error getting socket for node"<<endl;
            return;
        }
        HyDFSMessage response = receiveMessageFromSocket(socket);
        if(response.getType() == HyDFSMessage::HyDFSMessageType::READY_TO_RECEIVE_FILE){
            sendFileToSocket(filePath, socket);
        } else {
            cerr<<"Error sending file to node - "<< response.getTypeString()<<endl;
        }
        close(socket);
    }
    

    if(moveToReplica){
        secondaryFilesList.transferFileToList(fileName, myVersions);
    } else {
        for(int i=1;i<=myVersions;i++){
            deleteFile(getHyDFSFilesPath(filesDirectory, fileName, i));
        }
    }
    
}