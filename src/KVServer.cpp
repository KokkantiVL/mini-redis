#include "../include/KVServer.h"
#include "../include/CommandProcessor.h"
#include "../include/KVStore.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <cstring>
#include <signal.h>

// Global pointer for signal handling
static KVServer* serverInstance = nullptr;

void handleSignal(int sig) {
    if (serverInstance) {
        std::cout << "\nReceived signal " << sig << ", initiating shutdown...\n";
        serverInstance->stop();
    }
    exit(sig);
}

void KVServer::installSignalHandlers() {
    signal(SIGINT, handleSignal);
}

KVServer::KVServer(int port) : port_(port), listenSocket_(-1), isRunning_(true) {
    serverInstance = this;
    installSignalHandlers();
}

void KVServer::stop() {
    isRunning_ = false;
    if (listenSocket_ != -1) {
        // Persist database before shutdown
        if (KVStore::instance().saveToDisk("snapshot.kvdb"))
            std::cout << "Snapshot saved to snapshot.kvdb\n";
        else 
            std::cerr << "Error saving snapshot\n";
        close(listenSocket_);
    }
    std::cout << "Server shutdown complete!\n";
}

void KVServer::start() {
    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ < 0) {
        std::cerr << "Error creating socket\n";
        return;
    }

    int optVal = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error binding socket\n";
        return;
    }

    if (listen(listenSocket_, 10) < 0) {
        std::cerr << "Error listening on socket\n";
        return;
    } 

    std::cout << "KV Server listening on port " << port_ << "\n";

    std::vector<std::thread> clientThreads;
    CommandProcessor processor;

    while (isRunning_) {
        int clientSocket = accept(listenSocket_, nullptr, nullptr);
        if (clientSocket < 0) {
            if (isRunning_) 
                std::cerr << "Error accepting connection\n";
            break;
        }

        clientThreads.emplace_back([clientSocket, &processor](){
            char buffer[1024];
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesRead <= 0) break;
                std::string request(buffer, bytesRead);
                std::string response = processor.execute(request);
                send(clientSocket, response.c_str(), response.size(), 0);
            }
            close(clientSocket);
        });
    }
    
    for (auto& t : clientThreads) {
        if (t.joinable()) t.join();
    }

    // Final persistence before exit
    if (KVStore::instance().saveToDisk("snapshot.kvdb"))
        std::cout << "Snapshot saved to snapshot.kvdb\n";
    else 
        std::cerr << "Error saving snapshot\n";
}
