#ifndef KV_SERVER_H
#define KV_SERVER_H

#include <string>
#include <atomic>

class KVServer {
public:
    explicit KVServer(int port);
    void start();
    void stop();

private:
    int port_;
    int listenSocket_;
    std::atomic<bool> isRunning_;

    void installSignalHandlers();
};

#endif
