#include "../include/KVServer.h"
#include "../include/KVStore.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    int listenPort = 6379; // default Redis port
    if (argc >= 2) listenPort = std::stoi(argv[1]); 

    if (KVStore::instance().loadFromDisk("snapshot.kvdb"))
        std::cout << "Data loaded from snapshot.kvdb\n";
    else 
        std::cout << "No snapshot found or load failed; starting fresh.\n";

    KVServer server(listenPort);

    // Background persistence: save database every 300 seconds
    std::thread snapshotThread([](){
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300));
            if (!KVStore::instance().saveToDisk("snapshot.kvdb"))
                std::cerr << "Error saving snapshot\n";
            else 
                std::cout << "Snapshot saved to snapshot.kvdb\n";
        }
    });
    snapshotThread.detach();
    
    server.start();
    return 0;
}
