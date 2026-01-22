#include "Config.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>

const char* Config::DEFAULT_USER = "user";

Config::Config()
    : user(DEFAULT_USER)
    , extend(DEFAULT_EXTEND)
{
}

QString Config::getConfigDir()
{
#ifdef Q_OS_WIN
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // AppDataLocation already includes app name, go up one level
    QDir dir(basePath);
    dir.cdUp();
    return dir.filePath("GitSardine");
#elif defined(Q_OS_MACOS)
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return QDir(basePath).filePath("GitSardine");
#else
    // Linux and other Unix-like systems
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return QDir(basePath).filePath("gitsardine");
#endif
}

QString Config::getConfigPath()
{
    return QDir(getConfigDir()).filePath("config.json");
}

bool Config::exists()
{
    return QFile::exists(getConfigPath());
}

Result<VoidValue, QString> Config::load()
{
    QString path = getConfigPath();
    QFile file(path);

    if (!file.exists()) {
        return Result<VoidValue, QString>::Err("Config file does not exist: " + path);
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return Result<VoidValue, QString>::Err("Cannot open config file: " + path);
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        return Result<VoidValue, QString>::Err("JSON parse error: " + parseError.errorString());
    }

    if (!doc.isObject()) {
        return Result<VoidValue, QString>::Err("Config file must contain a JSON object");
    }

    QJsonObject obj = doc.object();

    // Parse paths (required)
    if (obj.contains("paths") && obj["paths"].isArray()) {
        paths.clear();
        QJsonArray pathsArray = obj["paths"].toArray();
        for (const QJsonValue& val : pathsArray) {
            if (val.isString()) {
                paths.append(val.toString());
            }
        }
    }

    // Parse user (required)
    if (obj.contains("user") && obj["user"].isString()) {
        user = obj["user"].toString();
    } else {
        user = DEFAULT_USER;
    }

    // Parse extend (optional, default 190)
    if (obj.contains("extend") && obj["extend"].isDouble()) {
        extend = obj["extend"].toInt();
    } else {
        extend = DEFAULT_EXTEND;
    }

    // Parse ignore patterns (optional)
    ignore.clear();
    if (obj.contains("ignore") && obj["ignore"].isArray()) {
        QJsonArray ignoreArray = obj["ignore"].toArray();
        for (const QJsonValue& val : ignoreArray) {
            if (val.isString()) {
                ignore.append(val.toString());
            } else if (val.isArray()) {
                // Array of strings - AND condition
                QStringList andPattern;
                QJsonArray innerArray = val.toArray();
                for (const QJsonValue& innerVal : innerArray) {
                    if (innerVal.isString()) {
                        andPattern.append(innerVal.toString());
                    }
                }
                ignore.append(QVariant(andPattern));
            }
        }
    }

    return OkVoid();
}

Result<VoidValue, QString> Config::save() const
{
    QString dirPath = getConfigDir();
    QDir dir;

    // Create directory if it doesn't exist
    if (!dir.exists(dirPath)) {
        if (!dir.mkpath(dirPath)) {
            return Result<VoidValue, QString>::Err("Cannot create config directory: " + dirPath);
        }
    }

    QJsonObject obj;

    // Write paths
    QJsonArray pathsArray;
    for (const QString& p : paths) {
        pathsArray.append(p);
    }
    obj["paths"] = pathsArray;

    // Write user
    obj["user"] = user;

    // Write extend
    obj["extend"] = extend;

    // Write ignore patterns
    QJsonArray ignoreArray;
    for (const QVariant& pattern : ignore) {
        if (pattern.type() == QVariant::String) {
            ignoreArray.append(pattern.toString());
        } else if (pattern.type() == QVariant::StringList) {
            QJsonArray innerArray;
            for (const QString& s : pattern.toStringList()) {
                innerArray.append(s);
            }
            ignoreArray.append(innerArray);
        }
    }
    obj["ignore"] = ignoreArray;

    QJsonDocument doc(obj);
    QString path = getConfigPath();
    QFile file(path);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return Result<VoidValue, QString>::Err("Cannot write config file: " + path);
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return OkVoid();
}

Result<VoidValue, QString> Config::createDefault()
{
    paths.clear();
    paths.append("/home/path/");
    user = DEFAULT_USER;
    extend = DEFAULT_EXTEND;
    ignore.clear();

    return save();
}
