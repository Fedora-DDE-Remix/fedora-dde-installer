#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

#include <QObject>

class KeyboardManager : public QObject
{
    Q_OBJECT

public:
    static KeyboardManager* instance();

    bool isCapslockOn();

signals:
    void capslockStatusChanged(bool on);

private:
    KeyboardManager();
    KeyboardManager(const KeyboardManager&) = delete;
};

#endif // KEYBOARDMANAGER_H
