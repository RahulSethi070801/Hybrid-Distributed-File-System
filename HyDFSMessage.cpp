#include "HyDFSMessage.h"
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <cassert>


const std::unordered_set<HyDFSMessage::HyDFSMessageType> HyDFSMessage::LIST_TYPE_MESSAGES = {
    HyDFSMessage::HyDFSMessageType::GET_FILE_LIST_RESPONSE
};

const std::unordered_set<HyDFSMessage::HyDFSMessageType> HyDFSMessage::FILE_NAME_VERSION_MESSAGES = {
    HyDFSMessage::HyDFSMessageType::
    GET_SHARD, 
    HyDFSMessage::HyDFSMessageType::PUT_FILE, 
};

const std::unordered_set<HyDFSMessage::HyDFSMessageType> HyDFSMessage::FILE_NAME_MESSAGES = {
    HyDFSMessage::HyDFSMessageType::GET_FILE, 
    HyDFSMessage::HyDFSMessageType::APPEND_FILE, 
    HyDFSMessage::HyDFSMessageType::FILE_ALREADY_EXISTS, 
    HyDFSMessage::HyDFSMessageType::READY_TO_RECEIVE_FILE, 
    HyDFSMessage::HyDFSMessageType::FILE_DOES_NOT_EXIST, 
    HyDFSMessage::HyDFSMessageType::MULTI_APPEND,
    HyDFSMessage::HyDFSMessageType::INVALIDATE_CACHE,
    HyDFSMessage::HyDFSMessageType::MERGE_FILE
};

const std::unordered_set<HyDFSMessage::HyDFSMessageType> HyDFSMessage::INFORMATIONAL_MESSAGES = {
    HyDFSMessage::HyDFSMessageType::ERROR_RECEIVING_MESSAGE,
    HyDFSMessage::HyDFSMessageType::GET_FILE_LIST, 
    HyDFSMessage::HyDFSMessageType::GET_ALL_FILE_LIST
};


HyDFSMessage::HyDFSMessage(HyDFSMessageType type, std::string fileName, int version) : type(type), fileName(fileName), version(version) {}

// HyDFSMessage::HyDFSMessage(HyDFSMessageType type, std::string fileName) : type(type), fileName(fileName) {}


// Converts HyDFSMessageType to a string
std::string HyDFSMessage::messageTypeToString(HyDFSMessageType type) {
    switch (type) {
        case HyDFSMessageType::GET_FILE:
            return "GET_FILE";
        case HyDFSMessageType::PUT_FILE:
            return "PUT_FILE";
        case HyDFSMessageType::GET_FILE_LIST:
            return "GET_FILE_LIST";
        case HyDFSMessageType::APPEND_FILE:
            return "APPEND_FILE";
        case HyDFSMessageType::FILE_ALREADY_EXISTS:
            return "FILE_ALREADY_EXISTS";
        case HyDFSMessageType::READY_TO_RECEIVE_FILE:
            return "READY_TO_RECEIVE_FILE";
        // case HyDFSMessageType::TRANSFER_FILE:
        //     return "TRANSFER_FILE";
        case HyDFSMessageType::GET_FILE_LIST_RESPONSE:
            return "GET_FILE_LIST_RESPONSE";
        case HyDFSMessageType::GET_SHARD:
            return "GET_SHARD";
        case HyDFSMessageType::FILE_DOES_NOT_EXIST:
            return "FILE_DOES_NOT_EXIST";
        case HyDFSMessageType::ERROR_RECEIVING_MESSAGE:
            return "ERROR_RECEIVING_MESSAGE";
        case HyDFSMessageType::MULTI_APPEND:
            return "MULTI_APPEND";
        case HyDFSMessageType::INVALIDATE_CACHE:
            return "INVALIDATE_CACHE";
        case HyDFSMessageType::MERGE_FILE:
            return "MERGE_FILE";
        case HyDFSMessageType::GET_ALL_FILE_LIST:
            return "GET_ALL_FILE_LIST";
        default:
            throw std::invalid_argument("Invalid hydfs message type");
    }
}

// Converts a string to HyDFSMessageType
HyDFSMessage::HyDFSMessageType HyDFSMessage::stringToMessageType(const std::string& typeStr) {
    if (typeStr == "GET_FILE") {
        return HyDFSMessageType::GET_FILE;
    } else if (typeStr == "PUT_FILE") {
        return HyDFSMessageType::PUT_FILE;
    } else if (typeStr == "GET_FILE_LIST") {
        return HyDFSMessageType::GET_FILE_LIST;
    } else if (typeStr == "GET_FILE_LIST_RESPONSE") {
        return HyDFSMessageType::GET_FILE_LIST_RESPONSE;
    } else if (typeStr == "APPEND_FILE") {
        return HyDFSMessageType::APPEND_FILE;
    } else if (typeStr == "FILE_ALREADY_EXISTS") {
        return HyDFSMessageType::FILE_ALREADY_EXISTS;
    } else if (typeStr == "READY_TO_RECEIVE_FILE") {
        return HyDFSMessageType::READY_TO_RECEIVE_FILE;
    // } else if (typeStr == "TRANSFER_FILE") {
    //     return HyDFSMessageType::TRANSFER_FILE;
    } else if (typeStr == "GET_SHARD") {
        return HyDFSMessageType::GET_SHARD;
    } else if (typeStr == "FILE_DOES_NOT_EXIST") {
        return HyDFSMessageType::FILE_DOES_NOT_EXIST;
    } else if (typeStr == "ERROR_RECEIVING_MESSAGE") {
        return HyDFSMessageType::ERROR_RECEIVING_MESSAGE;
    } else if (typeStr == "MULTI_APPEND") {
        return HyDFSMessageType::MULTI_APPEND;
    } else if (typeStr == "INVALIDATE_CACHE") {
        return HyDFSMessageType::INVALIDATE_CACHE;
    } else if (typeStr == "MERGE_FILE") {
        return HyDFSMessageType::MERGE_FILE;
    } else if (typeStr == "GET_ALL_FILE_LIST") {
        return HyDFSMessageType::GET_ALL_FILE_LIST;
    }
    else {
        throw std::invalid_argument("Invalid hydfs message type string");
    }
}

std::string HyDFSMessage::serializeMessage() {
    // Serialize the message into a string
    std::string serializedMessage = messageTypeToString(type);

    // add filelist to the serialized message
    if(LIST_TYPE_MESSAGES.find(type) != LIST_TYPE_MESSAGES.end()){
        if(filesList.size() > 0){
            for (const auto& filePair : filesList) {
                serializedMessage += "\n" + filePair.first + " " + std::to_string(filePair.second);
            }
        }
    } else if(FILE_NAME_VERSION_MESSAGES.find(type) != FILE_NAME_VERSION_MESSAGES.end()){
        serializedMessage += "\n" + fileName+" "+std::to_string(version);
    } else if(FILE_NAME_MESSAGES.find(type) != FILE_NAME_MESSAGES.end()){
        serializedMessage += "\n" + fileName;
    } else if (INFORMATIONAL_MESSAGES.find(type) != INFORMATIONAL_MESSAGES.end()) {
        // Do nothing
    } else {
        throw std::invalid_argument("Invalid message type");
    }

    return serializedMessage;
}

HyDFSMessage::HyDFSMessageType HyDFSMessage::getType() {
    return type;
}

std::string HyDFSMessage::getTypeString() {
    return messageTypeToString(type);
}

HyDFSMessage  HyDFSMessage::deserializeHyDFSMessage(const std::string& serialized){
    // Deserialize the message from a string
    std::istringstream iss(serialized);
    std::string typeStr;
    std::getline(iss, typeStr, '\n');
    HyDFSMessageType type = stringToMessageType(typeStr);
    
    
    if(LIST_TYPE_MESSAGES.find(type) != LIST_TYPE_MESSAGES.end()){
        std::vector<std::pair<std::string, int>> filesList;
        std::string fileName;
        std::string fileVersion;
        while(std::getline(iss, fileName, ' ')){
            std::getline(iss, fileVersion, '\n');
            std::istringstream iss2(fileVersion);
            int version;
            iss2 >> version;
            filesList.push_back({fileName, version});
        }
        return HyDFSMessage(type, filesList);
    } else if(FILE_NAME_VERSION_MESSAGES.find(type) != FILE_NAME_VERSION_MESSAGES.end()){
        std::string fileName;
        std::string fileVersion;
        std::getline(iss, fileName, ' ');
        std::getline(iss, fileVersion, '\n');
        std::istringstream iss2(fileVersion);
        int version;
        iss2 >> version;
        return HyDFSMessage(type, fileName, version);
    } else if(FILE_NAME_MESSAGES.find(type) != FILE_NAME_MESSAGES.end()){
        std::string fileName;
        std::getline(iss, fileName, '\n');
        return HyDFSMessage(type, fileName, 0);
    } else if (INFORMATIONAL_MESSAGES.find(type) != INFORMATIONAL_MESSAGES.end()) {
        return HyDFSMessage(type, "", 0);
    } else {
        throw std::invalid_argument("Invalid message type");
    }
   
}

std::string HyDFSMessage::getFileName() {
    return fileName;
}
int HyDFSMessage::getVersion() {
    return version;
}

HyDFSMessage::HyDFSMessage(HyDFSMessageType type, std::vector<std::pair<std::string, int>> filesList) : type(type), filesList(filesList) {}

std::vector<std::pair<std::string, int>> HyDFSMessage::getFilesList() {
    return filesList;
}

HyDFSMessage HyDFSMessage::getInformationalMessage(HyDFSMessageType type) {
    assert(INFORMATIONAL_MESSAGES.find(type) != INFORMATIONAL_MESSAGES.end());
    return HyDFSMessage(type, "", 0);
}

HyDFSMessage HyDFSMessage::getFileNameMessage(HyDFSMessageType type, std::string fileName) {
    assert(FILE_NAME_MESSAGES.find(type) != FILE_NAME_MESSAGES.end());
    return HyDFSMessage(type, fileName, 0);
}



HyDFSMessage HyDFSMessage::getFileNameVersionMessage(HyDFSMessageType type, std::string fileName, int version) {
    assert(FILE_NAME_VERSION_MESSAGES.find(type) != FILE_NAME_VERSION_MESSAGES.end());
    return HyDFSMessage(type, fileName, version);
}

HyDFSMessage HyDFSMessage::getFileListMessage(HyDFSMessageType type, std::vector<std::pair<std::string, int>> filesList) {
    assert(LIST_TYPE_MESSAGES.find(type) != LIST_TYPE_MESSAGES.end());
    return HyDFSMessage(type, filesList);
}

