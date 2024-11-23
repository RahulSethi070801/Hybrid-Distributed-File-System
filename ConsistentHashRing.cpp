#include "ConsistentHashRing.h"
#include "utils.h"


ConsistentHashRing::ConsistentHashRing() {}

void ConsistentHashRing::addNode(Node node) {
    lock_guard<mutex> lock(mtx);
    node.setRingHash(hashFunction(node.getNodeId()));
    if(nodeRing.size() == 0){
        selfNode = node;
    }
    nodeRing[node.getRingHash()] = node;
    nodesHashTable[node.getNodeId()] = node;
    cout << "Added node " << node.getNodeId() << " at position " << node.getRingHash() << "\n";
}

void ConsistentHashRing::removeNode(Node node) {
    lock_guard<mutex> lock(mtx);
    int ringHash = nodesHashTable[node.getNodeId()].getRingHash();
    nodeRing.erase(ringHash);
    nodesHashTable.erase(node.getNodeId());
    cout << "Removed node " << node.getNodeId() << " from position " << ringHash << "\n";
}


// overload << operator
ostream& operator<<(ostream& os, const ConsistentHashRing& ring) {
    for (const auto& pair : ring.nodeRing) {
        os << pair.first << " -> " << pair.second.getNodeId() << endl;
    }
    return os;
}

Node ConsistentHashRing::getPrimaryNodeForFile(int hash) {
    lock_guard<mutex> lock(mtx);
    auto it = nodeRing.lower_bound(hash);
    if (it == nodeRing.end()) {
        it = nodeRing.begin();
    }
    // cout << "Primary node for file with hash " << hash << " is " << it->first << " with node id " << it->second.getNodeId() << "\n";
    return it->second;
}

Node ConsistentHashRing::getNode(int hash) {
    lock_guard<mutex> lock(mtx);
    return nodeRing[hash];
}

Node ConsistentHashRing::getNode(string nodeId) {
    lock_guard<mutex> lock(mtx);
    return nodesHashTable[nodeId];
}

bool ConsistentHashRing::isPredecessor(string nodeId) {
    // consider the the map of nodes as a circular ring find the predecessor of the self node
    // if the predecessor is the node with the given nodeId, return true
    // else return false
    lock_guard<mutex> lock(mtx);
    auto it = nodeRing.find(selfNode.getRingHash());
    if (it == nodeRing.begin()) {
        it = prev(nodeRing.end());
    } else {
        it--;
    }
    return it->second.getNodeId() == nodeId;
}
    

Node ConsistentHashRing::getSelfNode() {
    lock_guard<mutex> lock(mtx);
    return selfNode;
}

vector<int> ConsistentHashRing::getKPredecessors(int k) {
    lock_guard<mutex> lock(mtx);
    vector<int> predecessors;
    auto it = nodeRing.find(selfNode.getRingHash());
    for (int i = 0; i < k; i++) {
        if (it == nodeRing.begin()) {
            it = prev(nodeRing.end());
        } else {
            it--;
        }
        if(it->second.getNodeId() == selfNode.getNodeId()){
            break;
        }
        predecessors.push_back(it->first);
    }
    return predecessors;
}

vector<int> ConsistentHashRing::getListOfNodes() {
    lock_guard<mutex> lock(mtx);
    vector<int> nodes;
    for (const auto& pair : nodeRing) {
        nodes.push_back(pair.first);
    }
    return nodes;
}

vector<int> ConsistentHashRing::getKSuccessorsOf(int k, int hash) {
    lock_guard<mutex> lock(mtx);
    vector<int> successors;
    auto it = nodeRing.lower_bound(hash);
    it++;
    for (int i = 0; i < k; i++) {
        if (it == nodeRing.end()) {
            it = nodeRing.begin();
        }
        if(it->second.getRingHash() == hash){
            break;
        }
        successors.push_back(it->first);
        it++;
    }
    return successors;
}

