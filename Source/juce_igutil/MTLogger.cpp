#include "MTLogger.h"

using namespace juce;
using namespace juce_igutil;

// Poison Pill definition.
const juce::String MTLogger::poisonPill("__MTLogger_PoisonPill__");

/**
 * Construct
 */
MTLogger::MTLogger(std::shared_ptr<juce::FileLogger> _pLogger) :
    pLogger(_pLogger)
{
    // Start the logger thread.
    // Note that calling a member function from the thread requires this 1-arg syntax:
    pLogger->logMessage("MTLogger - Constructor - starting log loop thread.");
    pLoggerThread.reset(new std::thread(&MTLogger::logLoop, this, LogLoopArgs{ queue, queueMutex, pLogger }));
}

/**
 * Destruct
 */
MTLogger::~MTLogger() 
{
    queueMutex.lock();
    queue.push(juce::String("MTLogger - Destructor - sending poisonPill."));
    queue.push(MTLogger::poisonPill);
    queueMutex.unlock();
    if (pLoggerThread->joinable())
        pLoggerThread->join();
    pLogger->logMessage("MTLogger - Destructor - done.");
}

/**
 * Log a debug message (TODO separate into debug/warn/error(?))
 */
void MTLogger::debug(const juce::String& message) {
    while ( !queueMutex.try_lock()) {
        std::this_thread::yield();
    }
    queue.push(message);
    queueMutex.unlock();
}

/**
 * Log an info message (currently just calls debug())
 * 
 */
void MTLogger::info(const juce::String& message) {
    debug(message);
}

/**
 * Log a warning message (currently just calls debug())
 */
void MTLogger::warning(const juce::String& message) {
    debug(message);
}

/**
 * Log an error message (currently just calls debug())
 */
void MTLogger::error(const juce::String& message) {
    debug(message);
}

/**
 * Loops forever, safely pulling from the queue and writing to 
 * the log.  Runs on its own thread, started from the 
 * constructor.
 */
void MTLogger::logLoop(LogLoopArgs args)
{
    bool done = false;
    while ( !done ) {
        
        // Don't be a processor hog
        std::this_thread::yield();

        if (args.queueMutex.try_lock()) {
            bool locked = true;
            while ( !done && !args.queue.empty() ) {
            
                // relock if needed
                if ( !locked ) {
                    args.queueMutex.lock();
                    locked = true;
                }

                // read and unlock
                const juce::String message(args.queue.front());
                args.queue.pop();
                args.queueMutex.unlock();
                locked = false;

                // Do the log or prepare to exit.  This is done outside of the locking 
                // to ensure that we don't hold the lock while doing this I/O.
                if (message == MTLogger::poisonPill) {
                    args.pLogger->logMessage(String("LOGGER: detected poison pill. Exiting..."));
                    done = true;
                }
                else {
                    args.pLogger->logMessage(message);
                }
            }
            
            // May still be unlocked if never got in the loop:
            if (locked) args.queueMutex.unlock();
        }
    }
}
