#include "settingswindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

SettingsWindow::SettingsWindow(QWidget *parent, QSettings *settings) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *label = new QLabel("This is a new window.");
        layout->addWidget(label);
        QStringList keys = settings->allKeys();
        QVBoxLayout *keys_box = new QVBoxLayout;
        QList<QLabel*> key_labels;
        for (int i=0;i<keys.size();i++){
            key_labels.append(new QLabel(keys[i]));
            keys_box->addWidget(key_labels[i]);
        }
        layout->addLayout(keys_box);
}

SettingsWindow::~SettingsWindow(){}
