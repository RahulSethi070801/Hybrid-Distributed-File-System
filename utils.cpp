#include <netdb.h> // Add this include for addrinfo
#include <unistd.h> // Add this include for close
#include <random>  // Add this include for std::random_device
#include <chrono>  // Add this include for std::chrono
#include "utils.h"
using namespace std;
int setupTCPSocket(char *port){

    struct addrinfo hints, *res, *tmp;
    int socketfd, status;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, port, &hints, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
    }

    tmp = res;
    
    while(tmp){

        if((socketfd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol)) == -1){
            perror("server: socket");
            tmp = tmp->ai_next;
            continue;
        }

        int opt = 1;
        setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if(::bind(socketfd, tmp->ai_addr, tmp->ai_addrlen) == -1){
            perror("server: bind");
            close(socketfd);
            tmp = tmp->ai_next;
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if(tmp == NULL){
        fprintf(stderr, "server: failed to bind tcp\n");
        exit(1);
    }
    cout<<"Socket binded\n";
    return socketfd;
}


int setupSocket(char *port) {
        struct addrinfo hints, *res, *tmp;
        int socketfd, status;
        
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_INET;
        hints.ai_flags = AI_PASSIVE;

        if((status = getaddrinfo(NULL, port, &hints, &res)) != 0){
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        }

        tmp = res;
        
        while(tmp){

            if((socketfd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol)) == -1){
                perror("server: socket");
                tmp = tmp->ai_next;
                continue;
            }

            if(::bind(socketfd, tmp->ai_addr, tmp->ai_addrlen) == -1){
                perror("server: bind");
                close(socketfd);
                tmp = tmp->ai_next;
                continue;
            }

            break;
        }

        freeaddrinfo(res);

        if(tmp == NULL){
            fprintf(stderr, "server: failed to bind udp\n");
            exit(1);
        }
        cout<<"Socket binded\n";
        return socketfd;
    }

struct sockaddr_in getAddrFromNode(Node node){

    // node.printNode();
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    // cout<<node.getNodeName()<<endl;
    int status = getaddrinfo(node.getNodeName(), node.getPort(), &hints, &res);
    if (status != 0) {
        std::cerr << "Error resolving hostname: "<<node.getNodeName()<<" :" << gai_strerror(status) << std::endl;
        exit(1);
    }

    struct sockaddr_in addr = *(struct sockaddr_in *)res->ai_addr;
    freeaddrinfo(res);
    return addr;
}

struct sockaddr_in getTCPAddrFromNode(Node node){
        // node.printNode();
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
    
        // cout<<node.getNodeName()<<endl;
        int status = getaddrinfo(node.getNodeName(), node.getTcpPort(), &hints, &res);
        if (status != 0) {
            std::cerr << "Error resolving hostname: "<<node.getNodeName()<<" :" << gai_strerror(status) << std::endl;
            exit(1);
        }
    
        struct sockaddr_in addr = *(struct sockaddr_in *)res->ai_addr;
        freeaddrinfo(res);
        return addr;
}


bool withProbability(double probability) {
    std::random_device rd;  // Non-deterministic random number generator
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_real_distribution<> dis(0.0, 1.0); // Uniform distribution between 0 and 1

    return dis(gen) < probability; // Perform action if random number is less than the probability
}

// unsigned long long generateHash(const std::string& str, int n) {
//     unsigned long long hash = 0;
//     const int p = 31; // A small prime number
//     const int m = 1e9 + 9; // A large prime modulus
//     unsigned long long p_pow = 1; // p^0

//     for (char c : str) {
//         hash = (hash + (c - 'a' + 1) * p_pow) % m;
//         p_pow = (p_pow * p) % m; // Increment p^i
//     }

//     return hash%(int)pow(2, n);
// }

char* getCurrentFullTimestamp() {
    char* timestamp = new char[24];
    time_t now = std::time(nullptr);
    tm* localTime = localtime(&now);
    auto now_ms = std::chrono::system_clock::now();
    auto duration = now_ms.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
    std::strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", localTime);
    std::snprintf(timestamp + 19, 5, ".%03d", static_cast<int>(millis));
    return timestamp;
}

std::string getCurrentTSinEpoch() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
    return std::to_string(value.count());
}


 int differenceWithCurrentEpoch(string ts){
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto epoch = now_ms.time_since_epoch();
        auto value = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);
        return value.count() - stoll(ts);
    }

int hashFunction(string key, int ringSize) {
        // Simple hash function for consistent hashing
        hash<string> hashFunc;
        
        size_t hashValue = hashFunc(key);
        return hashValue % ringSize;
    }