#ifndef HYDFS_MESSAGE_H
#define HYDFS_MESSAGE_H

#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_set>

class HyDFSMessage {
public:
    // Enum to represent different message types
    enum class HyDFSMessageType {
        GET_FILE,
        PUT_FILE,
        GET_FILE_LIST,
        GET_FILE_LIST_RESPONSE,
        APPEND_FILE,
        FILE_ALREADY_EXISTS,
        READY_TO_RECEIVE_FILE,
        // TRANSFER_FILE,
        GET_SHARD,
        FILE_DOES_NOT_EXIST,
        ERROR_RECEIVING_MESSAGE,
        MULTI_APPEND,
        INVALIDATE_CACHE,
        MERGE_FILE,
        GET_ALL_FILE_LIST
    };

    static const std::unordered_set<HyDFSMessageType> LIST_TYPE_MESSAGES, FILE_NAME_VERSION_MESSAGES, FILE_NAME_MESSAGES, INFORMATIONAL_MESSAGES;

    // Converts HyDFSMessageType to a string
    static std::string messageTypeToString(HyDFSMessageType type);
    // Converts a string to HyDFSMessageType
    static HyDFSMessageType stringToMessageType(const std::string& typeStr);
    std::string serializeMessage();
    static HyDFSMessage deserializeHyDFSMessage(const std::string& serialized);
    HyDFSMessageType getType();
    std::string getFileName();
    int getVersion();
    std::string getTypeString();
    // getter for filesList
    std::vector<std::pair<std::string, int>> getFilesList();

    // HyDFSMessage(HyDFSMessageType type, std::string fileName);
    static HyDFSMessage getInformationalMessage(HyDFSMessageType type);
    static HyDFSMessage getFileNameVersionMessage(HyDFSMessageType type, std::string fileName, int version);
    static HyDFSMessage getFileNameMessage(HyDFSMessageType type, std::string fileName);
    static HyDFSMessage getFileListMessage(HyDFSMessageType type, std::vector<std::pair<std::string, int>> filesList);




private:

    HyDFSMessage(HyDFSMessageType type, std::string fileName, int version = 0);
    HyDFSMessage(HyDFSMessageType type, std::vector<std::pair<std::string, int>> filesList);

    HyDFSMessageType type;
    std::string fileName;
    int version;
    std::vector<std::pair<std::string, int>> filesList;
};

#endif // HYDFS_MESSAGE_H