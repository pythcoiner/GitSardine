#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QVariantList>
#include "core/Result.h"

/**
 * Config - Configuration management for GitSardine
 *
 * Handles loading and saving of config.json with platform-specific paths:
 * - Linux: ~/.config/gitsardine/config.json
 * - Windows: %APPDATA%/GitSardine/config.json
 * - macOS: ~/Library/Application Support/GitSardine/config.json
 */
class Config {
public:
    Config();

    // Configuration fields
    QStringList paths;          // Root directories to scan for repos
    QString user;               // Username for branch naming validation
    int extend;                 // Pixels to add when window is extended
    QVariantList ignore;        // Patterns to filter from changes list

    // Get platform-specific config file path
    static QString getConfigPath();

    // Get config directory path
    static QString getConfigDir();

    // Load configuration from file
    Result<VoidValue, QString> load();

    // Save configuration to file
    Result<VoidValue, QString> save() const;

    // Create default configuration file
    Result<VoidValue, QString> createDefault();

    // Check if config file exists
    static bool exists();

private:
    static const int DEFAULT_EXTEND = 190;
    static const char* DEFAULT_USER;
};

#endif // CONFIG_H
