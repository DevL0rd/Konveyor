#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigSourceLines : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void errorsQuoteTheirLineWhateverTheLineBreak_data();
    void errorsQuoteTheirLineWhateverTheLineBreak();
    void aMissingFileNameFallsBackToTheDefault();
    void errorsPrintTheSourceLineOnlyWhenThereIsOne();
};

void TestConfigSourceLines::errorsQuoteTheirLineWhateverTheLineBreak_data()
{
    QTest::addColumn<QString>("lineBreak");
    QTest::newRow("line feed") << QStringLiteral("\n");
    QTest::newRow("crlf") << QStringLiteral("\r\n");
    QTest::newRow("cr") << QStringLiteral("\r");
    QTest::newRow("next line") << QStringLiteral("\u0085");
    QTest::newRow("form feed") << QStringLiteral("\f");
    QTest::newRow("line separator") << QStringLiteral(" ");
    QTest::newRow("paragraph separator") << QStringLiteral(" ");
}

void TestConfigSourceLines::errorsQuoteTheirLineWhateverTheLineBreak()
{
    QFETCH(QString, lineBreak);
    const QString text
        = QStringLiteral("layout {") + lineBreak + QStringLiteral("    gaps \"wide\"") + lineBreak + QStringLiteral("}") + lineBreak;
    const LoadError error = mustFail(text);
    QCOMPARE(error.location.line, 2);
    QCOMPARE(error.sourceLine, QStringLiteral("    gaps \"wide\""));
}

void TestConfigSourceLines::aMissingFileNameFallsBackToTheDefault()
{
    const auto result = loadString(QStringLiteral("layout { gaps \"wide\"; }"), QStringLiteral("/some/directory/"));
    QVERIFY(!result.has_value());
    QCOMPARE(result.error().location.file, QStringLiteral("config.kdl"));
}

void TestConfigSourceLines::errorsPrintTheSourceLineOnlyWhenThereIsOne()
{
    LoadError error;
    error.message = QStringLiteral("broken");
    error.location = Konveyor::Kdl::Location {QStringLiteral("a.kdl"), 3, 7};
    QCOMPARE(error.toString(), QStringLiteral("a.kdl:3:7: broken"));
    error.sourceLine = QStringLiteral("  gaps");
    QCOMPARE(error.toString(), QStringLiteral("a.kdl:3:7: broken\n  gaps"));
}

QTEST_MAIN(TestConfigSourceLines)
#include "test_config_sourcelines.moc"
