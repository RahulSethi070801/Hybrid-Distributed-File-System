#ifndef NODE_H
#define NODE_H

#include <string>
#include <iostream>

enum class Status { alive, suspected, failed, left };

class Node {
private:
    std::string nodeId;
    char *nodeName;
    char *port;
    char *tcpPort;
    Status status;
    int incarnationNumber;
    std::string lastUpdated;
    int ringHash;
public:
    Node();
    Node(std::string _nodeId, char *_nodeName, char* _port, char* _tcpPort, Status _status, int _incarnationNumber = 1);

    std::string getNodeId() const;
    char* getNodeName() const;
    char* getPort() const;
    char* getTcpPort() const;
    // get and set ring Hash
    int getRingHash() const;
    void setRingHash(int _ringHash);
    Status getStatus() const;
    int getIncarnationNumber() const;
    void setIncarnationNumber(int _incarnationNumber);
    void incrementIncarnationNumber();
    void setLastUpdated(const std::string& lastUpdated);
    std::string getLastUpdated() const;
    std::string serialise() const;
    static Node deserialiseNode(std::string serialised);
    void setStatus(Status _status);
    void setStatus(std::string _status);
    void printNode() const;


    static Node generateNode(char* port, char *tcpPort);
};

std::string convertStatusToString(Status status);
Status convertStringToStatus(std::string status);
long long getTimeDifference(std::string ts);
char* getCurrentTimestamp();

#endif // NODE_H