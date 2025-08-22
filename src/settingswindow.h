#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QObject>
#include <QWidget>
#include <QSettings>
class SettingsWindow : public QWidget
{
    Q_OBJECT
public:
    SettingsWindow(QWidget *parent = nullptr, QSettings *settings = nullptr);
    ~SettingsWindow();
};

#endif // SETTINGSWINDOW_H

