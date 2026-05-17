#include <QProcess>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>


bool command(const QString cmd, const QStringList &args, QString &log)
{
    QProcess *process = new QProcess;
    process->setProgram(cmd);
    process->setArguments(args);
    process->start();

    if (!process->waitForStarted()) {
        log = QString("QProcess::ProcessError: %1").arg(QString::number(process->error()));
        return false;
    }

    // Wait for process to finish without timeout.
    process->waitForFinished(-1);
    if (process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0) {
        log = process->readAllStandardOutput();
        return true;
    } else {
        // 获取具体的错误信息供前端显示
        log = process->readAllStandardError();
        return false;
    }
}

int main (int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    // 加密：echo -n '!!Uos123' | openssl enc -aes-128-cbc -a -salt -pass pass:'uos@123!'   输出：U2FsdGVkX18R5MhDer9vUhm7ASrOn0GCA0b/yp9hJ4k=
    // 解密：echo "U2FsdGVkX18R5MhDer9vUhm7ASrOn0GCA0b/yp9hJ4k=" | openssl aes128 -d -k "uos@123!" -base64 2>/dev/null   输出：!!Uos123

    // init command line
    QCommandLineOption decryptOption("d", "decode the password");
    QCommandLineOption passwadOption("p", "password.");
    passwadOption.setValueName("passwad to operate.");

    QCommandLineParser parser;
    parser.setApplicationDescription("deepin-installer-command-agent [-d] -p password");
    parser.addHelpOption();      // 添加帮助选项 （"-h" 或 "--help"）
    parser.addVersionOption();   // 添加版本选项 ("-v" 或 "--version")
    parser.addOption(decryptOption);
    parser.addOption(passwadOption);
    parser.process(app);

    QString password = "";
    if (!parser.isSet(passwadOption)) {
        parser.showHelp(-1);
    } else {
        password = parser.value(passwadOption).toLocal8Bit();
        if (password.isEmpty()) {
            parser.showHelp(-1);
        }
    }

    QString cmd, log;
    bool commandOk = false;
    if (parser.isSet(decryptOption)) {
        cmd = QString("echo \"%1\" | openssl aes128 -d -k \"uos@123!\" -base64 -pbkdf2 2>/dev/null").arg(password);
        commandOk = command("/bin/bash", {"-c", cmd}, log);
        log = log.trimmed();
    } else {
        QFile passwordFile("/tmp/deepin-installer-user-password.key");
        if (passwordFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            passwordFile.write(password.toLocal8Bit());
            passwordFile.close();
            cmd = QString("openssl enc -aes-128-cbc -a -salt -in /tmp/deepin-installer-user-password.key -pass pass:'uos@123!' -pbkdf2 2>/dev/null");
            commandOk = command("/bin/bash", {"-c", cmd}, log);
            log = log.trimmed();
            QFile::remove("/tmp/deepin-installer-user-password.key");
        } else {
            log = "treat password failed";
            commandOk = false;
        }
    }

    if (commandOk) {
        printf("%s", log.toLocal8Bit().data());
        return 0;
    } else {
        printf("error : %s", log.toLocal8Bit().data());
        return -1;
    }
}
