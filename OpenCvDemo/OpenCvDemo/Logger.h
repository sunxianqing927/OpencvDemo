#pragma once
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>


#define LOG_INFO(message) Logger::getInstance().log(__FILE__ ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + " - " + message, Logger::LogLevel::Info)
#define LOG_WARNING(message) Logger::getInstance().log(__FILE__ ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + " - " + message, Logger::LogLevel::Warning)
#define LOG_ERROR(message) Logger::getInstance().log(__FILE__ ":" + std::string(__func__) + ":" + std::to_string(__LINE__) + " - " + message, Logger::LogLevel::Error)

class Logger {
public:
    static Logger& getInstance();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    enum class LogLevel {
        Info,
        Warning,
        Error
    };

    void log(const std::string& message, LogLevel level);
private:
    Logger();
    ~Logger();

    std::string formatMessage(const std::string& message, LogLevel level);
    void processQueue();
    std::ofstream logFile;
    std::vector<std::string> logQueue;
    std::mutex mutex_;
    std::condition_variable messageReady;
    std::condition_variable messageComplete;
    bool isRunning = false;
    bool HasErrorMsg=false;
    std::thread logThread;
};

