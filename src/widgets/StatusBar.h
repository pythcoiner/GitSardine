#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QHBoxLayout>

/**
 * StatusBar - Status display with progress bar
 */
class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);

public slots:
    void setStatus(const QString& message);
    void setStatus(const QString& message, const QString& tooltip);
    void showProgress(bool visible);
    void setProgress(int percent);
    void clearStatus();

private:
    QLabel* m_label;
    QProgressBar* m_progress;
    QHBoxLayout* m_layout;

    void setupUi();
};

#endif // STATUSBAR_H
