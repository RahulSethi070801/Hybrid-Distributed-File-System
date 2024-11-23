#include<iostream>
#include <unistd.h>
#include <regex>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <fstream>
#include <condition_variable>
using namespace std;
using namespace std::chrono;

#include "MembershipList.cpp"
#include "Message.cpp"
#include "utils.h"
#include "HyDFS.h"



#define BUFFER_SIZE 4096
#define K 5
#define TTL 4
#define SUSPECT_TIMEOUT 4000
#define PING_TIMEOUT 1500
#define CYCLE_TIME 2000
#define DROP_PROBABILITY 0
#define PING_VALIDATOR_CYCLE_TIME 100



class Daemon {
private:
    char *port;
    int sockfd;
    MembershipList membershipList;
    std::atomic<bool> running;
    std::atomic<bool> isSuspicionMechanismEnabled;
    std::atomic<int> bytesReceived, bytesSent;
    HyDFS* dfsListener;
    // start timestamp

    std::queue<Message> infoQueue;
    std::mutex mtx_qinfo;
    std::condition_variable cv_qinfo;

    unordered_set<string> pingedNodes, suspectedNodes;
    std::mutex mtx_pingedNodes, mtx_suspectedNodes;

    string nodeId;
    Node selfNode;
    bool dropNextAck;

    vector<thread> threads;

    void sendTo(int sockfd, string message, struct sockaddr_in addr, MessageType type, Node toNode = Node()) {
        int numBytesSent = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr *)&addr, sizeof(addr));
        if (numBytesSent == -1) {
            perror("sendto failed");
        } else {
            bytesSent += numBytesSent;
            // get currennt timestamp in human readable format with milliseconds precision
            if(toNode.getNodeId() == ""){
                // cout << Node::getCurrentFullTimestamp() << ": Sent " << messageTypeToString(type) << " to " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << endl;
            } else {
                // cout << Node::getCurrentFullTimestamp() << ": Sent " << messageTypeToString(type) << " to " << toNode.getNodeName() << ":" << toNode.getPort() << endl;
            }
        }
    }

    
    void disseminateMessage(int sockfd, Message msg) {
        vector<Node> randomNodes = membershipList.getKShuffledNodes(K);
        if(msg.getType() == MessageType::FAILED || msg.getType() == MessageType::SUSPECT){
            randomNodes.push_back(msg.getNodesList().front());
        }
        for (const Node &n : randomNodes) {
            struct sockaddr_in nodeAddr = getAddrFromNode(n);
            sendTo(sockfd, msg.serializeMessage(), nodeAddr, msg.getType(), n);
        }
    }

    void suspectAndDisseminate(int sockfd, Node node, int ttl) {
        bool success = membershipList.markNodeSuspected(node);
        if(success){
            mtx_suspectedNodes.lock();
            suspectedNodes.insert(node.getNodeId());
            mtx_suspectedNodes.unlock();
            Node suspectedNode = membershipList.getNode(node.getNodeId());
            if(ttl <= 0) {
                return;
            }
            Message finalMessage = Message::createSuspectMessage(selfNode, {node}, ttl);
            disseminateMessage(sockfd, finalMessage);
        }
    }

    void setAliveAndDisseminate(int sockfd, Node node, int ttl) {
        bool success = membershipList.markNodeAlive(node);
        if(success){
            mtx_suspectedNodes.lock();
            suspectedNodes.erase(node.getNodeId());
            mtx_suspectedNodes.unlock();
            Node aliveNode = membershipList.getNode(node.getNodeId());
            if(ttl <= 0) {
                return;
            }
            Message finalMessage = Message::createAliveMessage(selfNode, {node}, ttl);
            disseminateMessage(sockfd, finalMessage);
        }
    }

    void failAndDisseminate(int sockfd, Node node, int ttl) {
        bool success = membershipList.removeNode(node);
        if(success){
            
            // desseminate only if you havent already marked the node as failed or left
            Node failedNode = membershipList.getNode(node.getNodeId());
            updateListener({failedNode});
            if(ttl <= 0) {
                return;
            }
            Message finalMessage = Message::createFailedMessage(selfNode, {node}, ttl);
            disseminateMessage(sockfd, finalMessage);
        }
    }

    void failedMessageProcessor(int sockfd, Message msg){
        if (msg.getNodesList().empty()) {
            perror("No Node To Fail");
        }
        else{
            Node failedNode = msg.getNodesList().front();
            if(failedNode.getNodeId() == selfNode.getNodeId()){
                cout<<"I am the one who is failed, killing myself\n";
                running = false;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                exit(0);
            }
            else{
                cout<<"Node "<<failedNode.getNodeName()<<" failed, disseminating info\n";
                failAndDisseminate(sockfd, failedNode, msg.getTTL()-1);
            }            
        }
    }

    void aliveMessageProcessor(int sockfd, Message msg){
        if(msg.getNodesList().empty()){
            perror("No Node To Mark Alive");
        }
        else {
            Node aliveNode = msg.getNodesList().front();
            if(aliveNode.getNodeId() == selfNode.getNodeId()){
                return;
            } else {
                setAliveAndDisseminate(sockfd, aliveNode, msg.getTTL()-1);
            }
        }
    }

    void pingMessageProcessor(int sockfd, Message msg) {
        Message ackMessage = Message::createAckMessageForPing(selfNode);
        Node senderNode = msg.getSenderNode();
        if(!membershipList.contains(senderNode)){
            membershipList.addNode(senderNode);
            updateListener({membershipList.getNode(senderNode.getNodeId())});
        }
        string serializedMessage = ackMessage.serializeMessage();
        struct sockaddr_in senderNode_addr = getAddrFromNode(senderNode);
        sendTo(sockfd, serializedMessage, senderNode_addr, MessageType::ACK, senderNode);
    }

    void toggleSuspicionMechanismAndDisseminate(int sockfd, bool isEnabled, int ttl) {
        if(isSuspicionMechanismEnabled == isEnabled) {
            return;
        }
        cout << getCurrentFullTimestamp() << "Toggling suspicion mechanism to " << isEnabled << endl;
        isSuspicionMechanismEnabled = isEnabled;
        if(ttl <= 0) {
            return;
        }
        Message susMessage = Message::createSuspectMechanismMessage(selfNode, isEnabled, ttl);
        disseminateMessage(sockfd, susMessage);
    }

    void leaveMessageProcessor(Message msg) {
        Node leftNode = msg.getSenderNode();
        if (membershipList.nodeExists(leftNode.getNodeId())) {
            if (!(leftNode.getStatus() == Status::failed || leftNode.getStatus() == Status::left)) {
                membershipList.markNodeLeft(leftNode);
                updateListener({membershipList.getNode(leftNode.getNodeId())});
            }
        }
    }

    void joinMessageProcessor(int sockfd, Message msg){
        vector<Node> nodes = membershipList.getAliveNodes();
        nodes.push_back(selfNode);
        Message ackMessage = Message::createAckMessage(selfNode, nodes);

        // Add the sender node to the membership list
        Node senderNode = msg.getSenderNode();
        membershipList.addNode(senderNode);
        updateListener({membershipList.getNode(senderNode.getNodeId())});
        
        // Serialize the message
        string serializedMessage = ackMessage.serializeMessage();
        struct sockaddr_in senderNode_addr = getAddrFromNode(senderNode);
        // Send the ACK message back to the sender
        sendTo(sockfd, serializedMessage, senderNode_addr, MessageType::ACK, senderNode);

        // sleep for 110ms to let everone know about the to let the node read the ack message
        // usleep(110000);

        // send current sus mechanism status to the new node
        Message susMessage = Message::createSuspectMechanismMessage(selfNode, isSuspicionMechanismEnabled, 1); 
        string serializedSusMessage = susMessage.serializeMessage();
        sendTo(sockfd, serializedSusMessage, senderNode_addr, susMessage.getType(), senderNode);
    }

    void suspectMessageProcessor(int sockfd, Message msg) {
        if (msg.getNodesList().empty()) {
            perror("No Node To Suspect");
        }
        else{
            Node suspectedNode = msg.getNodesList().front();
            if(suspectedNode.getNodeId() == selfNode.getNodeId()){
                cout<<getCurrentFullTimestamp()<<": I am the one who is suspected\n";
                if(suspectedNode.getIncarnationNumber() == selfNode.getIncarnationNumber()){
                    selfNode.incrementIncarnationNumber();
                    cout<<getCurrentFullTimestamp()<<": Incremented incarnation number\n";
                    selfNode.printNode();
                    Message finalMessage = Message::createAliveMessage(selfNode, {selfNode}, TTL);
                    disseminateMessage(sockfd, finalMessage);
                } 
                // else{
                //     Message finalMessage = Message::createAliveMessage(selfNode, {selfNode}, TTL);
                //     disseminateMessage(sockfd, finalMessage);
                // }
            }
            else{
                suspectAndDisseminate(sockfd, msg.getNodesList().front(), msg.getTTL()-1);
            } 
        }
    }

    void ackMessageProcessor(Message msg) {
        Node senderNode = msg.getSenderNode();
        membershipList.setLastUpdated(senderNode.getNodeId(), getCurrentTSinEpoch());
        mtx_pingedNodes.lock();
        pingedNodes.erase(senderNode.getNodeId());
        mtx_pingedNodes.unlock();
    }

    // message handler
    void infoMessageHandler(int sockfd) {
        while (running) {
            std::unique_lock<std::mutex> lock(mtx_qinfo);
            
            // Wait for an informational message
            if (cv_qinfo.wait_for(lock, std::chrono::seconds(1), [this] { return !infoQueue.empty(); })) {

                Message msg = infoQueue.front();
                infoQueue.pop();
          
                switch(msg.getType()) {
                    case MessageType::ACK : {
                        ackMessageProcessor(msg);
                        break;
                    }
                    case MessageType::JOIN : {
                        joinMessageProcessor(sockfd, msg);
                        break;
                    }
                    case MessageType::PING : {
                        pingMessageProcessor(sockfd, msg);
                        break;
                    }
                    case (MessageType::LEAVE) : {
                        leaveMessageProcessor(msg);
                        break;
                    }
                    case (MessageType::FAILED) : {
                        failedMessageProcessor(sockfd, msg);
                        break;
                    }
                    case (MessageType::ENABLE_SUSPECT_MECHANISM) : {
                        toggleSuspicionMechanismAndDisseminate(sockfd, true, msg.getTTL()-1);
                        break;
                    }
                    case (MessageType::DISABLE_SUSPECT_MECHANISM) : {
                        toggleSuspicionMechanismAndDisseminate(sockfd, false, msg.getTTL()-1);
                        break;
                    }
                    case (MessageType::SUSPECT) : {
                        suspectMessageProcessor(sockfd, msg);
                        break;
                    }
                    case (MessageType::ALIVE) : {
                        aliveMessageProcessor(sockfd, msg);
                        break;
                    }
                    default : {
                        cout<<"Default case\n";
                        break;
                    }
                }
            } else {
                // Handle timeout case if needed
            }
        }
    }
   
    void listener(int sockfd) {
        // start listening on socket sockfd
        char recvBuffer[BUFFER_SIZE];
        struct sockaddr_in senderAddr;
        socklen_t senderAddrLen = sizeof(senderAddr);
        cout<<"Listener started\n";
        struct timeval tv;
        // TODO: Set timeout to 2 seconds
        tv.tv_sec = 2;  // 2 seconds timeout
        tv.tv_usec = 0; // 0 microseconds

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("Error setting socket timeout");
        }

        while(running) {
            
            int numBytes = recvfrom(sockfd, recvBuffer, BUFFER_SIZE, 0, (struct sockaddr *)&senderAddr, &senderAddrLen);
            if(numBytes == -1){
                // perror("recvfrom listener");
                continue;
            }
            bytesReceived += numBytes;
            recvBuffer[numBytes] = '\0';
            
            Message msg = Message::deserializeMessage(recvBuffer);
            if( membershipList.nodeExists(msg.getSenderNode().getNodeId()) && 
                membershipList.getNode(msg.getSenderNode().getNodeId()).getStatus() == Status::failed){
                cout << getCurrentFullTimestamp() << "Node " << msg.getSenderNode().getNodeName() << ":" << msg.getSenderNode().getPort() << " is marked as failed, not processing the message" << endl;
                continue;
            }
            if (msg.getType() == MessageType::ACK && dropNextAck) {
                dropNextAck = false;
                cout << "Dropped ACK message from " << msg.getSenderNode().getNodeName() << ":" << msg.getSenderNode().getPort() << endl;
                continue;
            }
            // cout<< Node::getCurrentFullTimestamp() <<": Received " << messageTypeToString(msg.getType()) << " message from " << msg.getSenderNode().getNodeName() << ":" << msg.getSenderNode().getPort() << endl;
            
            std::unique_lock<std::mutex> lock(mtx_qinfo);
            infoQueue.push(msg);
            cv_qinfo.notify_all();
            
        }
    }

    void pinger(int sockfd) {
        while (running) {
            // Get the list of nodes from the membership list
            vector<Node> nodes = membershipList.getAliveShuffledNodes();

            // Iterate over the shuffled nodes and send ping messages
            for (const Node &node : nodes) {

                if (!running) break;
                if(membershipList.getStatus(node.getNodeId()) != Status::alive){
                    continue;
                }

                Message pingMessage = Message::createPingMessage(selfNode);
                string serializedMessage = pingMessage.serializeMessage();
                sockaddr_in nodeAddr = getAddrFromNode(node);
                sendTo(sockfd, serializedMessage, nodeAddr, MessageType::PING, node);

                membershipList.setLastUpdated(node.getNodeId(), getCurrentTSinEpoch());
                mtx_pingedNodes.lock();
                pingedNodes.insert(node.getNodeId());
                mtx_pingedNodes.unlock();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(CYCLE_TIME));
        }
    }

    void timeoutValidator(int sockfd) {
        while(running){
            std::this_thread::sleep_for(std::chrono::milliseconds(PING_VALIDATOR_CYCLE_TIME));
            mtx_pingedNodes.lock();
            vector<string> currPingedNodes(pingedNodes.begin(), pingedNodes.end());
            for (const string& nodeId : currPingedNodes) {

                string lastUpdated = membershipList.getLastUpdated(nodeId);
                if(lastUpdated == ""){
                    continue;
                }
                // cout<<"Timeout - "<<Node::differenceWithCurrentEpoch(lastUpdated)<<endl;
                if(differenceWithCurrentEpoch(lastUpdated) > PING_TIMEOUT) {
                    pingedNodes.erase(nodeId);
                    if(isSuspicionMechanismEnabled){
                        cout<<getCurrentFullTimestamp()<<": Suspecting node, I am deciding "<<nodeId<<endl;
                        suspectAndDisseminate(sockfd, membershipList.getNode(nodeId), TTL);
                    } else {
                        cout<<getCurrentFullTimestamp()<<": Failing node,  I am deciding "<<nodeId<<endl;
                        failAndDisseminate(sockfd, membershipList.getNode(nodeId), TTL);
                    }
                }

            }
            mtx_pingedNodes.unlock();

            if(isSuspicionMechanismEnabled){
                mtx_suspectedNodes.lock();
                vector<string> currSuspectedNodes(suspectedNodes.begin(), suspectedNodes.end());
                for (const string& nodeId : currSuspectedNodes) {
                    string lastUpdated = membershipList.getLastUpdated(nodeId);
                    if(lastUpdated == ""){
                        continue;
                    }
                    if(differenceWithCurrentEpoch(lastUpdated) > SUSPECT_TIMEOUT) {
                        suspectedNodes.erase(nodeId);
                        cout<<getCurrentFullTimestamp()<<": Failing node after sus timeout,  I am deciding "<<nodeId<<endl;
                        failAndDisseminate(sockfd, membershipList.getNode(nodeId), TTL);
                    }
                }
                mtx_suspectedNodes.unlock();
            }

        }
    }

    bool joinMembershipList(int sockfd, char *introducerName) {
        // Create a JOIN message
        Message joinMessage = Message::createJoinMessage(selfNode);

        // Serialize the message
        string serializedMessage = joinMessage.serializeMessage();

        // Send the JOIN message to the introducer
        struct addrinfo hints, *introducerAddr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        int status = getaddrinfo(introducerName, "4560", &hints, &introducerAddr);

        if (status != 0) {
            std::cerr << "Error resolving hostname: " << gai_strerror(status) << std::endl;
            return false;
        }

        sendTo(sockfd, serializedMessage, *(struct sockaddr_in *)introducerAddr->ai_addr, MessageType::JOIN);


        char recvBuffer[BUFFER_SIZE];
        struct timeval tv;
        tv.tv_sec = 5;  // 5 seconds timeout
        tv.tv_usec = 0; // 0 microseconds

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("Error setting socket timeout");
        }

        int numBytesReceived = recvfrom(sockfd, recvBuffer, BUFFER_SIZE, 0, nullptr, nullptr);
        recvBuffer[numBytesReceived] = '\0';
        if (numBytesReceived == -1) {
            perror("recv");
            return false;
        } else {
            recvBuffer[numBytesReceived] = '\0';
            Message ackMessage = Message::deserializeMessage(recvBuffer);
            vector<Node> nodes = ackMessage.getNodesList();
            membershipList.addNodes(nodes);
            updateListener({nodes});
        }

        freeaddrinfo(introducerAddr);
        return true;
    }

    

    
    

    void sendLeaveMessage(int sockfd) {
        // Create a LEAVE message
        Message leaveMessage = Message::createLeaveMessage(selfNode);

        // Serialize the message
        string serializedMessage = leaveMessage.serializeMessage();

        // Send the LEAVE message to all nodes in the membership list
        vector<Node> nodes = membershipList.getAliveNodes();
        for (const Node &node : nodes) {
            struct sockaddr_in nodeAddr = getAddrFromNode(node);
            sendTo(sockfd, serializedMessage, nodeAddr, MessageType::LEAVE, node);
        }
    }

public:
    

    Daemon(char *_port, char* tcpPort) {


        port = new char[strlen(_port) + 1];  // +1 for null terminator
        strcpy(port, _port);

        // convert isIntroducer to bool, False if NULL
        // bool isIntroducerBool = isIntroducer == NULL ? false : true;
        running = true;
        isSuspicionMechanismEnabled = true;
        bytesReceived = 0;
        bytesSent = 0;
        selfNode = Node::generateNode(port, tcpPort);

        dropNextAck = false;
        // start(isIntroducerBool, introducerName);
    }

    ~Daemon() {

        cout<<"Destructor called\n";
        cout<<selfNode.getNodeId()<<endl;
        delete[] port;
        
    }

    void addListener(HyDFS* listener){
        dfsListener = listener;
        dfsListener->setSelfNode(selfNode);
    }

    void updateListener(vector<Node> nodes){
        dfsListener->membershipUpdate(nodes);
    }

    void start(bool isIntroducer, char *introducerName) {
        cout<<"Starting Daemon\n";
        sockfd = setupSocket(port);
        if(!isIntroducer){
            if(!joinMembershipList(sockfd, introducerName)){
                cout<<"Failed to join membership list\n";
                return;
            } else {
                cout<<"Joined membership list\n";
            }
        }
        
        std::thread messageHandlerThread(&Daemon::infoMessageHandler, this, sockfd);
        threads.emplace_back(std::move(messageHandlerThread));
        std::thread listenerThread(&Daemon::listener, this, sockfd);
        threads.emplace_back(std::move(listenerThread));
        std::thread pingingThread(&Daemon::pinger, this, sockfd);
        threads.emplace_back(std::move(pingingThread));
        std::thread timeoutValidatorThread(&Daemon::timeoutValidator, this, sockfd);
        threads.emplace_back(std::move(timeoutValidatorThread));
        
        
        
        std::cout << "Daemon started on port " << port << std::endl;
    }

    void stopThreads(){
        running = false;
        for (thread &t : threads) {
            t.join();
        }
    }

    void runCommand(string command){
        // green cout
        cout<<"\033[1;32m";
        if(command == "leave" ){
            sendLeaveMessage(sockfd);
            stopThreads();
            close(sockfd);
        } else if (command == "show") {
            membershipList.printMembershipList();
        } else if (command == "enable_sus") {
            toggleSuspicionMechanismAndDisseminate(sockfd, true, TTL);
        } else if (command == "disable_sus") {
            toggleSuspicionMechanismAndDisseminate(sockfd, false, TTL);
        } else if (command == "status_sus") {
            std::cout << "SUSPICION MECHANISM STATUS: " << isSuspicionMechanismEnabled << std::endl;
        } else if (command == "list_mem"){
            membershipList.printMembershipList();
        } else if (command == "list_self") {
            cout<<"Self Node ------- \n";
            selfNode.printNode();
        } else if (command == "n"){
            cout<<"Alive nodes count: "<<membershipList.getAliveNodesCount()+1<<endl;
        } else if (command == "d"){
            dropNextAck = true;
        } else if (command == "b"){
            // color to blue
            cout<<"\033[1;34m";
            // auto end = high_resolution_clock::now();
            // auto duration = duration_cast<microseconds>(end - startTimestamp).count();
            cout<<"Bytes Received: "<<bytesReceived<<endl;
            cout<<"Bytes Sent: "<<bytesSent<<endl;
            // cout<<"Download bandwidth " << bytesReceived/((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTimestamp)).count()) << " bytes/sec" << endl;
            // cout<<"Upload bandwidth " << bytesSent/((std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTimestamp)).count()) << " bytes/sec" << endl;
            cout<<"\033[0m";
        } else if (command == "s"){
            // color to green
            cout<<"\033[1;32m";
            cout<<"Suspected nodes count: "<<suspectedNodes.size()<<endl;
            mtx_suspectedNodes.lock();
            for (const string& nodeId : suspectedNodes) {
                cout << nodeId << endl;
            }
            mtx_suspectedNodes.unlock();
            cout<<"\033[0m";
        } else {
            dfsListener->runCommand(command);
        }
        // change color to default
        cout<<"\033[0m";
        
    }
};



