#include "Node.h"
#include "utils.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <algorithm>
#include <random>

class MembershipList {
private:
    unordered_map<string, Node> nodes; // Assuming Node has a unique identifier of type string
    mutable std::mutex mtx; // Mutex to protect the nodes map

public:

    void addNode(const Node& node) {
        std::lock_guard<std::mutex> lock(mtx);
        nodes.insert(std::make_pair(node.getNodeId(), node)); // Assuming Node has a method getNodeId() that returns a unique identifier of type string
        nodes[node.getNodeId()].printNode();
    }

    void addNodes(const std::vector<Node>& nodeList) {
        std::lock_guard<std::mutex> lock(mtx);
        for (const Node& node : nodeList) {
            nodes[node.getNodeId()] = node;
            nodes[node.getNodeId()].printNode();
        }
    }

    bool removeNode(const Node& node) {
        std::lock_guard<std::mutex> lock(mtx);
        if(nodes.find(node.getNodeId()) != nodes.end() && (nodes[node.getNodeId()].getStatus() == Status::alive || nodes[node.getNodeId()].getStatus() == Status::suspected)
        && nodes[node.getNodeId()].getIncarnationNumber() == node.getIncarnationNumber()) {
            cout<<"DEBUG: Removing node - "<<node.getNodeId()<<endl;
            nodes[node.getNodeId()].setStatus(Status::failed);
            // red color cout
            cout<<"\033[1;31m"<<endl;
            nodes[node.getNodeId()].printNode();
            cout<<"\033[0m"<<endl;
            return true;
        }
        return false;
    }

    bool markNodeSuspected(const Node& node) {
        std::lock_guard<std::mutex> lock(mtx);
        if(nodes.find(node.getNodeId()) != nodes.end() && nodes[node.getNodeId()].getStatus() == Status::alive 
        && nodes[node.getNodeId()].getIncarnationNumber() == node.getIncarnationNumber()) {
            nodes[node.getNodeId()].setStatus(Status::suspected);
            
            // orange color cout
            cout<<"\033[1;33m"<<endl;
            nodes[node.getNodeId()].printNode();
            cout<<"\033[0m"<<endl;
            nodes[node.getNodeId()].setLastUpdated(getCurrentTSinEpoch());
            return true;
        }
        return false;
    }

    bool markNodeAlive(const Node& node) {
        std::lock_guard<std::mutex> lock(mtx);
        if(nodes.find(node.getNodeId()) != nodes.end() && nodes[node.getNodeId()].getIncarnationNumber() < node.getIncarnationNumber()) {
            cout<<"DEBUG: Marking node alive - "<<node.getNodeId()<<endl;
            nodes[node.getNodeId()].setStatus(Status::alive);
            nodes[node.getNodeId()].setIncarnationNumber(node.getIncarnationNumber());
            cout<<"NODE - "<<endl;
            // green color cout
            cout<<"\033[1;32m"<<endl;
            nodes[node.getNodeId()].printNode();
            cout<<"\033[0m"<<endl;
            return true;
        }
        return false;
    }

    void markNodeLeft(const Node& node) {
        std::lock_guard<std::mutex> lock(mtx);
        if(nodes.find(node.getNodeId()) != nodes.end()) {
            nodes[node.getNodeId()].setStatus(Status::left);
            nodes[node.getNodeId()].printNode();
        }
    }

    bool contains(const Node& node) const {
        std::lock_guard<std::mutex> lock(mtx);
        return nodes.find(node.getNodeId()) != nodes.end();
    }

    std::vector<Node> getAliveNodes() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Node> nodeList;
        for (const auto& pair : nodes) {
            if (pair.second.getStatus() == Status::alive) {
                nodeList.push_back(pair.second);
            }
        }
        return nodeList;
    }

    // get shuffled list of nodes
    std::vector<Node> getAliveShuffledNodes() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Node> nodeList;
        for (const auto& pair : nodes) {
            if (pair.second.getStatus() == Status::alive) {
                nodeList.push_back(pair.second);
            }
        }
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(nodeList.begin(), nodeList.end(), g);
        return nodeList;
    }

    // set last updated time of a node given nodeId
    void setLastUpdated(const std::string& nodeId, const std::string& lastUpdated) {
        std::lock_guard<std::mutex> lock(mtx);
        if (nodes.find(nodeId) != nodes.end()) {
            auto it = nodes.find(nodeId);
            if (it != nodes.end()) {
                it->second.setLastUpdated(lastUpdated);
            }
        }
    }

    string getLastUpdated(const std::string& nodeId)  {
        std::lock_guard<std::mutex> lock(mtx);
        if (nodes.find(nodeId) != nodes.end()) {
            return nodes[nodeId].getLastUpdated();
        }
        return "";
    }


    std::vector<Node> getKShuffledNodes(int k) const {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Node> nodeList;
        for (const auto& pair : nodes) {
            if (pair.second.getStatus() == Status::alive || pair.second.getStatus() == Status::suspected) {
                nodeList.push_back(pair.second);
            }
        }
        if ((int)nodeList.size() > k) {
            nodeList.resize(k);
        }
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(nodeList.begin(), nodeList.end(), g);
        return nodeList;
    }


      

    // set status of a node given nodeId
    void setStatus(const std::string& nodeId, Status status) {
        std::lock_guard<std::mutex> lock(mtx);
        if (nodes.find(nodeId) != nodes.end()) {
            auto it = nodes.find(nodeId);
            if (it != nodes.end()) {
                it->second.setStatus(status);
            }
        }
    }

    // get status of a node given nodeId
    Status getStatus(const std::string& nodeId)  {
        std::lock_guard<std::mutex> lock(mtx);
        if (nodes.find(nodeId) != nodes.end()) {
            return nodes[nodeId].getStatus();
        }
        return Status::failed;
    }

    // bool function to check if node exists in the list
    bool nodeExists(const std::string& nodeId) const {
        std::lock_guard<std::mutex> lock(mtx);
        return nodes.find(nodeId) != nodes.end();
    }

    Node getNode(const std::string& nodeId) {
        std::lock_guard<std::mutex> lock(mtx);
        return nodes[nodeId];
    }


    // serialize the membership list
    std::string serialize() const {
        std::lock_guard<std::mutex> lock(mtx);
        std::string serialized;
        for (const auto& pair : nodes) {
            serialized += pair.second.serialise() + "\n";
        }
        return serialized;
    }

    // deserialize the membership list and return membership list
    static MembershipList deserialize(const std::string& serialized) {
        MembershipList ml;
        std::istringstream ss(serialized);
        std::string line;
        while (std::getline(ss, line, '\n')) {
            Node node = Node::deserialiseNode(line);
            ml.addNode(node);
        }
        return ml;
    }

    //copy constructor
    MembershipList(const MembershipList& ml) {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& pair : ml.nodes) {
            nodes.insert(pair);
        }
    }

    //empty constructor
    MembershipList() {}

    //print membership list
    void printMembershipList() const {
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& pair : nodes) {
            pair.second.printNode();
        }
    }

    // assignment operator
    MembershipList& operator=(const MembershipList& ml) {
        std::lock_guard<std::mutex> lock(mtx);
        if (this != &ml) {
            nodes.clear();
            for (const auto& pair : ml.nodes) {
                nodes.insert(pair);
            }
        }
        return *this;
    }

    // get size of the membership list
    int getSize() const {
        std::lock_guard<std::mutex> lock(mtx);
        return nodes.size();
    }

    int getAliveNodesCount() const {
        std::lock_guard<std::mutex> lock(mtx);
        int count = 0;
        for (const auto& pair : nodes) {
            if (pair.second.getStatus() == Status::alive) {
                count++;
            }
        }
        return count;
    }




};
