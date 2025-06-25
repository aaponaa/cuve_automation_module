#include "logger.h"

void Logger::log(LogLevel level, const String& module, const String& message) {
  if (level < minLevel) return;
  
  // Générer une clé pour le rate limiting
  String rateLimitKey = generateRateLimitKey(level, module, message);
  
  // Vérifier si ce log doit être rate-limité
  if (shouldRateLimit(rateLimitKey)) {
    return; // Log supprimé à cause du rate limiting
  }
  
  LogEntry entry;
  entry.timestamp = millis();
  entry.level = level;
  entry.module = module;
  entry.message = message;
  
  // Ajouter à la liste des logs
  logs.push_back(entry);
  
  // Limiter la taille du buffer
  if (logs.size() > MAX_LOG_ENTRIES) {
    logs.erase(logs.begin());
  }
  
  // Affichage sur le port série si activé
  if (serialEnabled) {
    String timestamp = formatTimestamp(entry.timestamp);
    String levelStr = levelToString(level);
    Serial.println("[" + timestamp + "] " + levelStr + " - " + module + ": " + message);
  }
}

String Logger::generateRateLimitKey(LogLevel level, const String& module, const String& message) {
  // Créer une clé basée sur le module, le niveau et un hash simple du message
  // Pour les messages similaires (comme "No valid readings for X seconds"), 
  // on peut extraire juste le début du message
  String messageKey = message;
  
  // Pour les messages de type "No valid readings for X seconds", 
  // on normalise pour traiter tous ces messages comme identiques
  if (message.startsWith("No valid readings for")) {
    messageKey = "No valid readings for X seconds";
  }
  
  return String(level) + ":" + module + ":" + messageKey;
}

bool Logger::shouldRateLimit(const String& key) {
  unsigned long currentTime = millis();
  
  auto it = rateLimitMap.find(key);
  if (it == rateLimitMap.end()) {
    // Premier log de ce type, l'autoriser
    RateLimitInfo info;
    info.lastLogTime = currentTime;
    info.count = 1;
    info.suppressedCount = 0;
    rateLimitMap[key] = info;
    return false;
  }
  
  RateLimitInfo& info = it->second;
  
  // Vérifier si on est dans une nouvelle fenêtre de temps
  if (currentTime - info.lastLogTime > RATE_LIMIT_WINDOW_MS) {
    // Nouvelle fenêtre - si on avait supprimé des logs, afficher un résumé
    if (info.suppressedCount > 0) {
      addSuppressedLogSummary(key, info);
    }
    
    // Réinitialiser pour la nouvelle fenêtre
    info.lastLogTime = currentTime;
    info.count = 1;
    info.suppressedCount = 0;
    return false;
  }
  
  // Dans la même fenêtre - vérifier si on dépasse la limite
  if (info.count >= MAX_LOGS_PER_WINDOW) {
    info.suppressedCount++;
    return true; // Rate limiting actif
  }
  
  info.count++;
  return false;
}

void Logger::addSuppressedLogSummary(const String& key, RateLimitInfo& info) {
  // Extraire le module de la clé (format: "level:module:message")
  int firstColon = key.indexOf(':');
  int secondColon = key.indexOf(':', firstColon + 1);
  
  if (firstColon != -1 && secondColon != -1) {
    String module = key.substring(firstColon + 1, secondColon);
    String summaryMessage = "Rate limiting: " + String(info.suppressedCount) + " similar messages suppressed in last 2 seconds";
    
    // Ajouter le résumé directement sans rate limiting
    LogEntry entry;
    entry.timestamp = millis();
    entry.level = LOG_WARNING;
    entry.module = module;
    entry.message = summaryMessage;
    
    logs.push_back(entry);
    
    if (logs.size() > MAX_LOG_ENTRIES) {
      logs.erase(logs.begin());
    }
    
    if (serialEnabled) {
      String timestamp = formatTimestamp(entry.timestamp);
      Serial.println("[" + timestamp + "] WARN - " + module + ": " + summaryMessage);
    }
  }
}

String Logger::levelToString(LogLevel level) {
  switch (level) {
    case LOG_DEBUG: return "DEBUG";
    case LOG_INFO: return "INFO";
    case LOG_WARNING: return "WARN";
    case LOG_ERROR: return "ERROR";
    default: return "UNKNOWN";
  }
}

String Logger::formatTimestamp(unsigned long timestamp) {
  unsigned long totalSeconds = timestamp / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long milliseconds = timestamp % 1000;
  
  char buffer[16];
  sprintf(buffer, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, milliseconds);
  return String(buffer);
}

String Logger::getLogsAsJson(int count) {
  String json = "[";
  
  int startIndex = max(0, (int)logs.size() - count);
  
  for (int i = startIndex; i < logs.size(); i++) {
    if (i > startIndex) json += ",";
    
    LogEntry& entry = logs[i];
    json += "{";
    json += "\"time\":\"" + formatTimestamp(entry.timestamp) + "\",";
    json += "\"level\":\"" + levelToString(entry.level) + "\",";
    json += "\"module\":\"" + entry.module + "\",";
    json += "\"message\":\"" + entry.message + "\"";
    json += "}";
  }
  
  json += "]";
  return json;
}