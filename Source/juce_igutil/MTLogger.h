// Multi-threaded Logger Class
// 
// This class helps provide logging in threads that require very fast response time.
// Keeps a thread safe in-memory store of messages to log as well as a separate thread 
// that reads from the store and does the logging.

#pragma once

#include <JuceHeader.h>
#include <shared_mutex>

namespace juce_igutil {

class MTLogger {

public:
    static const juce::String poisonPill;

public:
    MTLogger(std::shared_ptr<juce::FileLogger> _pLogger);

    virtual ~MTLogger();

    // logging functions
    void debug(const juce::String& message);
    void info(const juce::String& message);
    void warning(const juce::String& message);
    void error(const juce::String& message);

private:

    // Logger.  Only use while the worker thread is not created nor joined.
    std::shared_ptr<juce::FileLogger> pLogger;

    // Logging worker thread and function
    std::unique_ptr<std::thread> pLoggerThread;
    struct LogLoopArgs {
        std::queue<juce::String, std::deque<juce::String>>& queue;
        std::shared_mutex & queueMutex;
        std::shared_ptr<juce::FileLogger> pLogger;
    };
    void logLoop(LogLoopArgs args);
    
    // Message queue and protection
    std::queue<juce::String, std::deque<juce::String>> queue;
    std::shared_mutex queueMutex;
};

}

