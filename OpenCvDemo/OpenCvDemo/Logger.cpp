#include "pch.h"
#include "Logger.h"

#include <iostream>
#include <chrono>

#define _CRT_SECURE_NO_WARNINGS

// 获取单例实例

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message, LogLevel level) {
    std::string logMessage = formatMessage(message, level);
    std::unique_lock<std::mutex> lock(mutex_);
    logQueue.push_back(logMessage);
    messageReady.notify_one();
    if (level == LogLevel::Error) {
        HasErrorMsg = true;
        messageComplete.wait(lock, [this]() { return logQueue.empty(); });
    }
}

Logger::Logger() : logFile("log.txt", std::ios::app), isRunning(true), logThread(&Logger::processQueue, this) {}

Logger::~Logger() {
    isRunning = false;
    messageReady.notify_all();
    if (logThread.joinable()) {
        logThread.join();
    }
}

std::string formatCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::tm timeStruct;
    localtime_s(&timeStruct, &now_time_t);

    std::ostringstream oss;
    oss << std::put_time(&timeStruct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::formatMessage(const std::string& message, LogLevel level) {
    std::string levelStr;
    switch (level) {
    case LogLevel::Info: levelStr = "INFO"; break;
    case LogLevel::Warning: levelStr = "WARNING"; break;
    case LogLevel::Error: levelStr = "ERROR"; break;
    }

    std::ostringstream oss;
    oss << formatCurrentTime() << " [" << levelStr << "] " << message;
    return oss.str();
}

void Logger::processQueue() {
    std::vector<std::string> logsToWrite;
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            messageReady.wait(lock, [this]() { return !logQueue.empty() || !isRunning; });
            if (!isRunning && logQueue.empty()) {
                break;
            }
            logsToWrite.swap(logQueue);
            if (!HasErrorMsg) {
                lock.unlock();
            }

            for (const auto& logMessage : logsToWrite) {
                logFile << logMessage << std::endl;
            }
            logsToWrite.clear();
            if (HasErrorMsg) {
                HasErrorMsg = false;
                messageComplete.notify_one();
            }
        }
    }
}
