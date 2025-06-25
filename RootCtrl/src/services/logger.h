#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <vector>
#include <map>
#include <time.h>

enum LogLevel {
  LOG_DEBUG = 0,
  LOG_INFO = 1,
  LOG_WARNING = 2,
  LOG_ERROR = 3
};

struct LogEntry {
  unsigned long timestamp;
  LogLevel level;
  String module;
  String message;
};

// Structure pour le rate limiting
struct RateLimitInfo {
  unsigned long lastLogTime;
  unsigned int count;
  unsigned int suppressedCount;
};

class Logger {
private:
  static const size_t MAX_LOG_ENTRIES = 1000;
  static const unsigned long RATE_LIMIT_WINDOW_MS = 2000; // 2 secondes
  static const unsigned int MAX_LOGS_PER_WINDOW = 1; // Max 1 logs identiques par 10 secondes
  
  std::vector<LogEntry> logs;
  LogLevel minLevel = LOG_DEBUG;
  bool serialEnabled = true;
  
  // Map pour le rate limiting: clé = module + niveau + message hash
  std::map<String, RateLimitInfo> rateLimitMap;
  
public:
  static Logger& getInstance() {
    static Logger instance;
    return instance;
  }
  
  void log(LogLevel level, const String& module, const String& message);
  void debug(const String& module, const String& message) { log(LOG_DEBUG, module, message); }
  void info(const String& module, const String& message) { log(LOG_INFO, module, message); }
  void warning(const String& module, const String& message) { log(LOG_WARNING, module, message); }
  void error(const String& module, const String& message) { log(LOG_ERROR, module, message); }
  
  void setMinLevel(LogLevel level) { minLevel = level; }
  LogLevel getMinLevel() const { return minLevel; }
  void setSerialEnabled(bool enabled) { serialEnabled = enabled; }
  
  String getLogsAsJson(int count = 50);
  void clear() { 
    logs.clear(); 
    rateLimitMap.clear(); // Nettoyer aussi le rate limiting
  }
  size_t getLogCount() const { return logs.size(); }
  
private:
  Logger() {}
  String levelToString(LogLevel level);
  String formatTimestamp(unsigned long timestamp);
  String generateRateLimitKey(LogLevel level, const String& module, const String& message);
  bool shouldRateLimit(const String& key);
  void addSuppressedLogSummary(const String& key, RateLimitInfo& info);
};

// Macros pour faciliter l'utilisation
#define LOG_DEBUG(module, msg) Logger::getInstance().debug(module, msg)
#define LOG_INFO(module, msg) Logger::getInstance().info(module, msg)
#define LOG_WARNING(module, msg) Logger::getInstance().warning(module, msg)
#define LOG_ERROR(module, msg) Logger::getInstance().error(module, msg)

#endif // LOGGER_H