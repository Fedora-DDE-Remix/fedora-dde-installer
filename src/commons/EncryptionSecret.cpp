#include "EncryptionSecret.h"

#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <cerrno>
#include <fcntl.h>
#include <linux/memfd.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
struct HeldSecret {
    QMutex mutex;
    int fd = -1;
    ~HeldSecret() { if (fd >= 0) ::close(fd); }
};

HeldSecret &secret()
{
    static HeldSecret value;
    return value;
}
}

void EncryptionSecret::wipe(QByteArray &value)
{
    volatile char *bytes = value.data();
    for (qsizetype i = 0; i < value.size(); ++i)
        bytes[i] = 0;
    value.clear();
}

bool EncryptionSecret::store(QByteArray &passphrase, QString &error)
{
    clear();
    error.clear();
    if (passphrase.size() < MinimumBytes || passphrase.size() > MaximumBytes
            || passphrase.contains('\0') || passphrase.contains('\r') || passphrase.contains('\n')) {
        wipe(passphrase);
        error = QObject::tr("Use a passphrase of 8 to 1024 UTF-8 bytes without line breaks.");
        return false;
    }

    const int fd = static_cast<int>(::syscall(SYS_memfd_create,
                "fedora-dde-crypt-passphrase", MFD_CLOEXEC | MFD_ALLOW_SEALING));
    bool ok = fd >= 0;
    qsizetype written = 0;
    while (ok && written < passphrase.size()) {
        const ssize_t result = ::write(fd, passphrase.constData() + written, passphrase.size() - written);
        if (result < 0 && errno == EINTR)
            continue;
        if (result <= 0) {
            ok = false;
            break;
        }
        written += result;
    }
    wipe(passphrase);
    if (ok)
        ok = ::fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE) == 0;
    if (!ok) {
        if (fd >= 0)
            ::close(fd);
        error = QObject::tr("Could not keep the encryption passphrase in protected memory. Please try again.");
        return false;
    }

    auto &held = secret();
    QMutexLocker lock(&held.mutex);
    if (held.fd >= 0)
        ::close(held.fd);
    held.fd = fd;
    return true;
}

bool EncryptionSecret::available()
{
    auto &held = secret();
    QMutexLocker lock(&held.mutex);
    return held.fd >= 0;
}

void EncryptionSecret::clear()
{
    auto &held = secret();
    QMutexLocker lock(&held.mutex);
    if (held.fd >= 0)
        ::close(held.fd);
    held.fd = -1;
}

int EncryptionSecret::prepareProcess(QProcess &process, QString &error)
{
    error.clear();
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.remove("FEDORA_DDE_CRYPT_FD");
    process.setProcessEnvironment(environment);

    auto &held = secret();
    QMutexLocker lock(&held.mutex);
    if (held.fd < 0)
        return -1;
    const int fd = ::fcntl(held.fd, F_DUPFD_CLOEXEC, 3);
    if (fd < 0) {
        error = QObject::tr("Could not pass the encryption key to the installation process.");
        return -1;
    }
    environment.insert("FEDORA_DDE_CRYPT_FD", QString::number(fd));
    process.setProcessEnvironment(environment);
    process.setChildProcessModifier([fd] {
        // This runs after fork: use only async-signal-safe system calls.
        if (::fcntl(fd, F_SETFD, 0) < 0)
            ::_exit(126);
    });
    return fd;
}
