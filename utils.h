#ifndef UTILS_H
#define UTILS_H

// Includes required libraries for the function
#include <netdb.h>
#include <sys/socket.h>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <string>
#include "Node.h"

#define RING_SIZE pow(2, 10)

// Function prototype for setting up a TCP socket
int setupTCPSocket(char *port);
int setupSocket(char *port);
struct sockaddr_in getAddrFromNode(Node node);
struct sockaddr_in getTCPAddrFromNode(Node node);
bool withProbability(double probability);
// unsigned long long generateHash(const std::string& str, int n=10);
char* getCurrentFullTimestamp();
std::string getCurrentTSinEpoch();
int differenceWithCurrentEpoch(std::string ts);
int hashFunction(std::string key, int m = RING_SIZE);

#endif // UTILS_H
