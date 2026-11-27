#ifndef ENCRYPTIONSECRET_H
#define ENCRYPTIONSECRET_H

#include <QByteArray>
#include <QString>

class QProcess;

// The partition plan contains only the encryption choice. The passphrase is
// owned by an anonymous, sealed, close-on-exec descriptor until partitioning.
class EncryptionSecret
{
public:
    static constexpr qint64 MetadataMiB = 32;
    static constexpr int MinimumBytes = 8;
    static constexpr int MaximumBytes = 1024;

    static bool store(QByteArray &passphrase, QString &error);
    static bool available();
    static void clear();
    static void wipe(QByteArray &value);

    // Returns a parent-owned CLOEXEC duplicate, or -1 when no key is set.
    // The caller closes this duplicate after the child finishes. Only this
    // child clears CLOEXEC; unrelated processes cannot inherit the secret.
    static int prepareProcess(QProcess &process, QString &error);
};

#endif
