#include "Node.h"
#include "utils.h"
#include <sstream>
#include <cstring>
#include <chrono>
#include <ctime>
#include <unistd.h>

std::string convertStatusToString(Status status) {
    switch (status) {
        case Status::alive: return "alive";
        case Status::suspected: return "suspected";
        case Status::failed: return "failed";
        case Status::left: return "left";
        default: return "unknown";
    }
}

Status convertStringToStatus(std::string status) {
    if (status == "alive") return Status::alive;
    if (status == "suspected") return Status::suspected;
    if (status == "failed") return Status::failed;
    if (status == "left") return Status::left;
    std::cerr << "Error: Invalid status string '" << status << "' provided." << std::endl;
    return Status::failed;
}

Node::Node() {}

Node::Node(std::string _nodeId, char *_nodeName, char* _port, char* _tcpPort, Status _status, int _incarnationNumber)
    : nodeId(_nodeId), nodeName(_nodeName), port(_port), tcpPort(_tcpPort), status(_status), incarnationNumber(_incarnationNumber) {}

std::string Node::getNodeId() const { return nodeId; }
char* Node::getNodeName() const { return nodeName; }
char* Node::getPort() const { return port; }
Status Node::getStatus() const { return status; }
int Node::getIncarnationNumber() const { return incarnationNumber; }
void Node::setIncarnationNumber(int _incarnationNumber) { incarnationNumber = _incarnationNumber; }
void Node::incrementIncarnationNumber() { incarnationNumber += 1; }
void Node::setLastUpdated(const std::string& _lastUpdated) { lastUpdated = _lastUpdated; }
std::string Node::getLastUpdated() const { return lastUpdated; }

std::string Node::serialise() const {
    std::string serialised = nodeId + "$" + nodeName + "$" + port + "$" + tcpPort + "$" + std::to_string(incarnationNumber) + "$";
    serialised += convertStatusToString(status);
    return serialised;
}

Node Node::deserialiseNode(std::string serialised) {
    std::stringstream ss(serialised);
    std::string nodeId, nodeName, port, statusStr, incarnationNumberStr, tcpPort;
    getline(ss, nodeId, '$');
    getline(ss, nodeName, '$');
    getline(ss, port, '$');
    getline(ss, tcpPort, '$');
    getline(ss, incarnationNumberStr, '$');
    int incarnationNumber;
    try {
        incarnationNumber = std::stoi(incarnationNumberStr);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid argument for incarnation number: " << e.what() << std::endl;
        incarnationNumber = 1; // Default value
    } catch (const std::out_of_range& e) {
        std::cerr << "Error: Incarnation number out of range: " << e.what() << std::endl;
        incarnationNumber = 1; // Default value
    }
    getline(ss, statusStr, '$');
    Status status = convertStringToStatus(statusStr);
    char *port_str = new char[port.length() + 1];
    std::strcpy(port_str, port.c_str());
    char *tcpPort_str = new char[tcpPort.length() + 1];
    std::strcpy(tcpPort_str, tcpPort.c_str());
    char *nodeName_str = new char[nodeName.length() + 1];
    std::strcpy(nodeName_str, nodeName.c_str());
    Node node(nodeId, nodeName_str, port_str, tcpPort_str, status, incarnationNumber);
    return node;
}

void Node::setStatus(Status _status) { status = _status; }

void Node::setStatus(std::string _status) {
    if (_status == "alive") status = Status::alive;
    else if (_status == "suspected") status = Status::suspected;
    else if (_status == "failed") status = Status::failed;
    else if (_status == "left") status = Status::left;
    else std::cerr << "Error: Invalid status string '" << _status << "' provided." << std::endl;
}

void Node::printNode() const {
    std::cout << "Node --------" << std::endl;
    std::cout << "Node ID: " << nodeId << std::endl;
    std::cout << "Node Name: " << nodeName << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "TCP Port: " << tcpPort << std::endl;
    std::cout << "Status: " << convertStatusToString(status) << std::endl;
    std::cout << "Incarnation Number: " << incarnationNumber << std::endl;
    std::cout << "Last Updated: " << lastUpdated << std::endl;
    long long timeDiff = lastUpdated!=""? differenceWithCurrentEpoch(lastUpdated) : 0;
    std::cout << "Time since last updated: " << timeDiff << std::endl;
}

long long getTimeDifference(std::string ts) {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    return value.count() - std::stoll(ts);
}

char* getCurrentTimestamp() {
    char* timestamp = new char[9];
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);
    std::strftime(timestamp, 9, "%H:%M:%S", localTime);
    return timestamp;
}

Node Node::generateNode(char* port, char *tcpPort) {
    char hostbuffer[256];
    if (gethostname(hostbuffer, sizeof(hostbuffer)) == -1) {
        std::cerr << "Error getting hostname" << std::endl;
        exit(1);
    }
    char * timestamp = getCurrentTimestamp();
    char* result = new char[std::strlen(hostbuffer) + std::strlen(port) + std::strlen(timestamp) + 3];
    std::strcpy(result, hostbuffer);
    std::strcat(result, "#");
    std::strcat(result, port);
    std::strcat(result, "#");
    std::strcat(result, timestamp);
    std::string resultStr(result);
    char* hostbufferCopy = new char[std::strlen(hostbuffer) + 1];
    std::strcpy(hostbufferCopy, hostbuffer);
    return Node(resultStr, hostbufferCopy, port, tcpPort, Status::alive);
}

int Node::getRingHash() const {
    return ringHash;
}

void Node::setRingHash(int _ringHash){
    ringHash = _ringHash;
}

char* Node::getTcpPort() const {
    return tcpPort;
}