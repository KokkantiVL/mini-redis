#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include <string>
#include <vector>

// Parse RESP protocol input into command tokens
std::vector<std::string> parseProtocol(const std::string& input);

class CommandProcessor {
public:
    CommandProcessor();
    // Execute a command and return RESP-formatted response
    std::string execute(const std::string& rawInput);
};

#endif
