/**
 * Tree structure test for GitSardine
 * Verifies FolderTreeModel correctly builds hierarchical tree from directories
 */

#include <QGuiApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QDebug>
#include <QStandardItem>
#include "models/FolderTreeModel.h"

class TreeTest {
public:
    bool run() {
        qDebug() << "=== FolderTreeModel Test ===\n";

        // Create temporary test directory structure
        QTemporaryDir tempDir;
        if (!tempDir.isValid()) {
            qCritical() << "FAIL: Could not create temp directory";
            return false;
        }

        QString basePath = tempDir.path();
        qDebug() << "Test directory:" << basePath;

        // Create structure:
        // basePath/
        //   ├── folder1/
        //   │   ├── repo1/ (.git)
        //   │   └── repo2/ (.git)
        //   ├── folder2/
        //   │   └── subfolder/
        //   │       └── repo3/ (.git)
        //   └── repo_at_root/ (.git)

        QDir base(basePath);
        base.mkpath("folder1/repo1/.git");
        base.mkpath("folder1/repo2/.git");
        base.mkpath("folder2/subfolder/repo3/.git");
        base.mkpath("repo_at_root/.git");

        qDebug() << "\nExpected structure:";
        qDebug() << "  " << QDir(basePath).dirName() << "/ (root)";
        qDebug() << "    folder1/ (dir with 2 children)";
        qDebug() << "      repo1/ (repo)";
        qDebug() << "      repo2/ (repo)";
        qDebug() << "    folder2/ (dir with 1 child)";
        qDebug() << "      subfolder/ (dir with 1 child)";
        qDebug() << "        repo3/ (repo)";
        qDebug() << "    repo_at_root/ (repo)";

        // Create and scan model
        FolderTreeModel model;
        model.scanPaths({basePath});

        // Test 1: Check total repo count
        QStringList allRepos = model.getAllRepoPaths();
        qDebug() << "\n--- Test 1: Repo count ---";
        qDebug() << "Found" << allRepos.count() << "repositories (expected 4)";
        if (allRepos.count() != 4) {
            qCritical() << "FAIL: Expected 4 repos, got" << allRepos.count();
            return false;
        }
        qDebug() << "PASS";

        // Test 2: Check root item exists and has children
        qDebug() << "\n--- Test 2: Root item ---";
        QStandardItem* invisibleRoot = model.invisibleRootItem();
        if (invisibleRoot->rowCount() != 1) {
            qCritical() << "FAIL: Expected 1 root item, got" << invisibleRoot->rowCount();
            return false;
        }

        FolderItem* root = dynamic_cast<FolderItem*>(invisibleRoot->child(0));
        if (!root) {
            qCritical() << "FAIL: Could not get root FolderItem";
            return false;
        }
        qDebug() << "Root:" << root->text() << "isRepo:" << root->isRepo << "children:" << root->rowCount();
        qDebug() << "PASS";

        // Test 3: Check root has 3 children (folder1, folder2, repo_at_root)
        qDebug() << "\n--- Test 3: Root children ---";
        if (root->rowCount() != 3) {
            qCritical() << "FAIL: Root should have 3 children, got" << root->rowCount();
            return false;
        }
        qDebug() << "Root has 3 children (expected)";
        qDebug() << "PASS";

        // Test 4: Check folder1 has 2 repo children
        qDebug() << "\n--- Test 4: folder1 structure ---";
        FolderItem* folder1 = nullptr;
        for (int i = 0; i < root->rowCount(); ++i) {
            FolderItem* child = dynamic_cast<FolderItem*>(root->child(i));
            if (child && child->text() == "folder1") {
                folder1 = child;
                break;
            }
        }
        if (!folder1) {
            qCritical() << "FAIL: Could not find folder1";
            return false;
        }
        if (folder1->isRepo) {
            qCritical() << "FAIL: folder1 should not be a repo";
            return false;
        }
        if (folder1->rowCount() != 2) {
            qCritical() << "FAIL: folder1 should have 2 children, got" << folder1->rowCount();
            return false;
        }
        qDebug() << "folder1: isRepo=" << folder1->isRepo << "children=" << folder1->rowCount();
        qDebug() << "PASS";

        // Test 5: Check nested structure (folder2/subfolder/repo3)
        qDebug() << "\n--- Test 5: Nested structure ---";
        FolderItem* folder2 = nullptr;
        for (int i = 0; i < root->rowCount(); ++i) {
            FolderItem* child = dynamic_cast<FolderItem*>(root->child(i));
            if (child && child->text() == "folder2") {
                folder2 = child;
                break;
            }
        }
        if (!folder2 || folder2->rowCount() != 1) {
            qCritical() << "FAIL: folder2 should have 1 child";
            return false;
        }
        FolderItem* subfolder = dynamic_cast<FolderItem*>(folder2->child(0));
        if (!subfolder || subfolder->text() != "subfolder") {
            qCritical() << "FAIL: Could not find subfolder";
            return false;
        }
        if (subfolder->rowCount() != 1) {
            qCritical() << "FAIL: subfolder should have 1 child, got" << subfolder->rowCount();
            return false;
        }
        FolderItem* repo3 = dynamic_cast<FolderItem*>(subfolder->child(0));
        if (!repo3 || !repo3->isRepo) {
            qCritical() << "FAIL: repo3 should be a repo";
            return false;
        }
        qDebug() << "folder2 -> subfolder -> repo3 structure verified";
        qDebug() << "PASS";

        // Test 6: Check repo_at_root is a repo with no children
        qDebug() << "\n--- Test 6: repo_at_root ---";
        FolderItem* repoAtRoot = nullptr;
        for (int i = 0; i < root->rowCount(); ++i) {
            FolderItem* child = dynamic_cast<FolderItem*>(root->child(i));
            if (child && child->text() == "repo_at_root") {
                repoAtRoot = child;
                break;
            }
        }
        if (!repoAtRoot) {
            qCritical() << "FAIL: Could not find repo_at_root";
            return false;
        }
        if (!repoAtRoot->isRepo) {
            qCritical() << "FAIL: repo_at_root should be a repo";
            return false;
        }
        if (repoAtRoot->rowCount() != 0) {
            qCritical() << "FAIL: repo_at_root should have no children";
            return false;
        }
        qDebug() << "repo_at_root: isRepo=" << repoAtRoot->isRepo << "children=" << repoAtRoot->rowCount();
        qDebug() << "PASS";

        qDebug() << "\n=== ALL TESTS PASSED ===";
        return true;
    }
};

int main(int argc, char *argv[]) {
    // Use offscreen platform for headless testing
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);

    TreeTest test;
    return test.run() ? 0 : 1;
}
