#include "StatusBar.h"

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void StatusBar::setupUi()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // Status label
    m_label = new QLabel(this);
    m_label->setMinimumHeight(31);
    m_label->setMouseTracking(true);

    // Progress bar
    m_progress = new QProgressBar(this);
    m_progress->setMinimumHeight(31);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);

    m_layout->addWidget(m_label);
    m_layout->addWidget(m_progress);

    setLayout(m_layout);
}

void StatusBar::setStatus(const QString& message)
{
    m_label->setText(message);
    m_label->setToolTip("");
    m_label->setVisible(true);
    m_progress->setVisible(false);
}

void StatusBar::setStatus(const QString& message, const QString& tooltip)
{
    m_label->setText(message);
    m_label->setToolTip(tooltip);
    m_label->setVisible(true);
    m_progress->setVisible(false);
}

void StatusBar::showProgress(bool visible)
{
    m_progress->setVisible(visible);
    m_label->setVisible(!visible);
    if (visible) {
        m_progress->setValue(0);
    }
}

void StatusBar::setProgress(int percent)
{
    m_progress->setValue(percent);
}

void StatusBar::clearStatus()
{
    m_label->setText("");
    m_label->setToolTip("");
}
