#include <string>
#include <unordered_set>
using namespace std;

enum MessageType { JOIN, PING, ACK, FAILED, LEAVE, SUSPECT, ENABLE_SUSPECT_MECHANISM, DISABLE_SUSPECT_MECHANISM, ALIVE};

MessageType stringToMessageType(std::string type) {
    if (type == "JOIN") {
        return MessageType::JOIN;
    } else if (type == "PING") {
        return MessageType::PING;
    } else if (type == "ACK") {
        return MessageType::ACK;
    } else if (type == "FAILED") {
        return MessageType::FAILED;
    } else if (type == "LEAVE") {
        return MessageType::LEAVE;
    } else if (type == "SUSPECT") {
        return MessageType::SUSPECT;
    } else if (type == "ENABLE_SUSPECT_MECHANISM") {
        return MessageType::ENABLE_SUSPECT_MECHANISM;
    } else if (type == "DISABLE_SUSPECT_MECHANISM") {
        return MessageType::DISABLE_SUSPECT_MECHANISM;
    } else if (type == "ALIVE") {
        return MessageType::ALIVE;
    }
    else {
        throw std::invalid_argument("Invalid message type");
    }
}

string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::JOIN:
            return "JOIN";
        case MessageType::PING:
            return "PING";
        case MessageType::ACK:
            return "ACK";
        case MessageType::FAILED:
            return "FAILED";
        case MessageType::LEAVE:
            return "LEAVE";
        case MessageType::SUSPECT:
            return "SUSPECT";
        case MessageType::ENABLE_SUSPECT_MECHANISM:
            return "ENABLE_SUSPECT_MECHANISM";
        case MessageType::DISABLE_SUSPECT_MECHANISM:
            return "DISABLE_SUSPECT_MECHANISM";
        case MessageType::ALIVE:
            return "ALIVE";
        default:
            throw std::invalid_argument("Invalid membership message type");
    }
}

const unordered_set<MessageType> messageHasPayload = {MessageType::ACK, MessageType::FAILED, MessageType::SUSPECT, MessageType::ALIVE};

class Message {

    MessageType type;
    vector<Node> nodesList;
    Node senderNode;
    int TTL;

public:
    MessageType getType() {
        return type;
    }   


    Message(MessageType type, Node senderNode, int ttl=0) { 
        this->type = type;
        this->senderNode = senderNode;
        this->TTL = ttl;
    }


    Message(MessageType type, Node senderNode, vector<Node> ml, int ttl=0) {
        this->type = type;
        this->nodesList = ml;
        this->senderNode = senderNode;
        this->TTL = ttl;
    }

    // getter and setter for sender node

    Node getSenderNode() {
        return senderNode;
    }

    void setSenderNode(Node senderNode) {
        this->senderNode = senderNode;
    }

    // getter and setter for TTL

    int getTTL() {
        return TTL;
    }

    void setTTL(int TTL) {
        this->TTL = TTL;
    }


    string serializeMessage() {
        string serialised = messageTypeToString(type) + "\n";
        serialised += senderNode.serialise() + "\n";
        serialised += to_string(TTL) + "\n";
        if (messageHasPayload.find(type) != messageHasPayload.end()) {
            for (Node node : nodesList) {
                serialised += node.serialise() + "\n";
            }
        }
        
        return serialised;
    }

    static Message deserializeMessage(string serialised) {
        std::istringstream ss(serialised);
        std::string line;
        std::getline(ss, line, '\n');
        MessageType type = stringToMessageType(line);
        std::getline(ss, line, '\n');
        Node senderNode = Node::deserialiseNode(line);
        std::getline(ss, line, '\n');
        int ttl;
        try {
            ttl = std::stoi(line);
        } catch (const std::invalid_argument& e) {
            throw std::runtime_error("Failed to deserialize TTL: invalid argument");
        } catch (const std::out_of_range& e) {
            throw std::runtime_error("Failed to deserialize TTL: out of range");
        }
        if (messageHasPayload.find(type) != messageHasPayload.end()) {
            vector<Node> nodesList;
            while (std::getline(ss, line, '\n')) {
                Node node = Node::deserialiseNode(line);
                nodesList.push_back(node);
            }
            
            return Message(type, senderNode, nodesList, ttl);
        }
        return Message(type, senderNode, ttl);
    }

    static Message createPingMessage(Node senderNode) {
        return Message(MessageType::PING, senderNode);
    }

    static Message createJoinMessage(Node senderNode) {
        return Message(MessageType::JOIN, senderNode);
    }

    static Message createAckMessage(Node senderNode, vector<Node> ml) {
        return Message(MessageType::ACK, senderNode, ml);
    }

    static Message createAckMessageForPing(Node senderNode) {
        return Message(MessageType::ACK, senderNode);
    }

    static Message createLeaveMessage(Node senderNode) {
        return Message(MessageType::LEAVE, senderNode);
    }

    static Message createFailedMessage(Node senderNode, vector<Node> ml, int ttl) {
        return Message(MessageType::FAILED, senderNode, ml, ttl);
    }

    static Message createSuspectMessage(Node senderNode, vector<Node> ml, int ttl) {
        return Message(MessageType::SUSPECT, senderNode, ml, ttl);
    }

    static Message createEnableSuspectMechanismMessage(Node senderNode, int ttl) {
        return Message(MessageType::ENABLE_SUSPECT_MECHANISM, senderNode, ttl);
    }

    static Message createDisableSuspectMechanismMessage(Node senderNode, int ttl) {
        return Message(MessageType::DISABLE_SUSPECT_MECHANISM, senderNode, ttl);
    }

    static Message createSuspectMechanismMessage(Node senderNode, bool isEnabled, int ttl) {
        if(isEnabled){
            return createEnableSuspectMechanismMessage(senderNode, ttl);
        }
        else {
            return createDisableSuspectMechanismMessage(senderNode, ttl);
        }
    }

    static Message createAliveMessage(Node senderNode, vector<Node> ml, int ttl) {
        return Message(MessageType::ALIVE, senderNode, ml, ttl);
    }


    void printMessage() {
        cout << "Message --------------------------" << endl;
        cout << "Message Type: " << messageTypeToString(type) << endl;
        cout << "Sender Node: " << senderNode.getNodeName() << endl;
        cout << "Sender Port: " << senderNode.getPort() << endl;
        cout << "TTL: " << TTL << endl;
        if (messageHasPayload.find(type) != messageHasPayload.end()) {
            cout << "Nodes List: " << endl;
            for (Node node : nodesList) {
                node.printNode();
            }
        }
        cout << "----------------------------" << endl;
    }

    // get membership list
    vector<Node> getNodesList() {
        return nodesList;
    }
};
