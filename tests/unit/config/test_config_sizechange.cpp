#include "config/sizechange.h"

#include <QTest>

using namespace Konveyor::Config;

class TestConfigSizeChange : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesEveryForm_data();
    void parsesEveryForm();
    void rejectsBadInput_data();
    void rejectsBadInput();
};

void TestConfigSizeChange::parsesEveryForm_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("integer");
    QTest::addColumn<SizeChangeKind>("kind");
    QTest::addColumn<double>("value");
    QTest::newRow("set fixed") << QStringLiteral("800") << true << SizeChangeKind::SetFixed << 800.0;
    QTest::newRow("adjust fixed up") << QStringLiteral("+10") << true << SizeChangeKind::AdjustFixed << 10.0;
    QTest::newRow("adjust fixed down") << QStringLiteral("-10") << true << SizeChangeKind::AdjustFixed << -10.0;
    QTest::newRow("set proportion") << QStringLiteral("50%") << true << SizeChangeKind::SetProportion << 50.0;
    QTest::newRow("adjust proportion") << QStringLiteral("+12.5%") << true << SizeChangeKind::AdjustProportion << 12.5;
    QTest::newRow("fractional fixed") << QStringLiteral("2.5") << false << SizeChangeKind::SetFixed << 2.5;
    QTest::newRow("fractional adjust") << QStringLiteral("-0.25") << false << SizeChangeKind::AdjustFixed << -0.25;
}

void TestConfigSizeChange::parsesEveryForm()
{
    QFETCH(QString, text);
    QFETCH(bool, integer);
    QFETCH(SizeChangeKind, kind);
    QFETCH(double, value);
    const auto change = parseSizeChange(text, integer);
    QVERIFY(change.has_value());
    QCOMPARE(change->kind, kind);
    QCOMPARE(change->value, value);
    QCOMPARE(*change, (SizeChange {kind, value}));
}

void TestConfigSizeChange::rejectsBadInput_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("integer");
    QTest::addColumn<QString>("message");
    QTest::newRow("trailing after percent") << QStringLiteral("10%x") << true
                                            << QStringLiteral("trailing characters after '%' are not allowed");
    QTest::newRow("only percent") << QStringLiteral("%") << true << QStringLiteral("value is missing");
    QTest::newRow("empty") << QString() << true << QStringLiteral("value is missing");
    QTest::newRow("not a number") << QStringLiteral("abc") << true << QStringLiteral("error parsing value");
    QTest::newRow("fraction as integer") << QStringLiteral("2.5") << true << QStringLiteral("error parsing value");
    QTest::newRow("not a fraction") << QStringLiteral("2.x") << false << QStringLiteral("error parsing value");
}

void TestConfigSizeChange::rejectsBadInput()
{
    QFETCH(QString, text);
    QFETCH(bool, integer);
    QFETCH(QString, message);
    const auto change = parseSizeChange(text, integer);
    QVERIFY(!change.has_value());
    QCOMPARE(change.error(), message);
}

QTEST_GUILESS_MAIN(TestConfigSizeChange)
#include "test_config_sizechange.moc"
