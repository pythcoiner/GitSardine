#ifndef REPOMODEL_H
#define REPOMODEL_H

#include <QString>
#include "git/GitStatus.h"

/**
 * RepoModel - Data model for a single repository
 */
class RepoModel {
public:
    QString name;
    QString path;
    RepoStatus status;
    bool isRepo;

    RepoModel()
        : isRepo(false)
    {}

    RepoModel(const QString& n, const QString& p, bool repo = true)
        : name(n)
        , path(p)
        , isRepo(repo)
    {}
};

#endif // REPOMODEL_H
