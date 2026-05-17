#ifndef INSTALLTEST_H
#define INSTALLTEST_H

#include <QObject>
#include <QtTest/QTest>

class InstallTest : public QObject {
    Q_OBJECT

public:
    InstallTest() = default;
    ~InstallTest();

private slots:
    void test_languages_list();
    void test_languages_license();

    void test_partitions_option();
    void test_partitions_fullDiks();
};

#endif // INSTALLTEST_H
