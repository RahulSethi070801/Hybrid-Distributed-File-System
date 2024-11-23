#ifndef RING_H
#define RING_H


#include <map>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "Node.h"
using namespace std;


class ConsistentHashRing {
private:
    map<int, Node> nodeRing; 
    unordered_map<string, Node> nodesHashTable;
    Node selfNode;
    mutable std::mutex mtx;
public:
    ConsistentHashRing();
    void addNode(Node node);
    void removeNode(Node node);
    Node getPrimaryNodeForFile(int hash);
    Node getNode(int hash);
    Node getNode(string nodeId);
    Node getSelfNode();
    // overload << operator
    friend ostream& operator<<(ostream& os, const ConsistentHashRing& ring);
    bool isPredecessor(string nodeId);
    vector<int> getKPredecessors(int k); 
    vector<int> getListOfNodes();
    vector<int> getKSuccessorsOf(int k, int hash);

};

ostream& operator<<(ostream& os, const ConsistentHashRing& ring);

#endif // RING_H