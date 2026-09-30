#include "configtesthelpers.h"

#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTest>

using namespace Konveyor::Config;

namespace
{

constexpr int Iterations = 4000;

QString readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

const QStringList &replacementValues()
{
    static const QStringList values {QStringLiteral("0"), QStringLiteral("-1"), QStringLiteral("1"), QStringLiteral("0.5"),
        QStringLiteral("-0.0"), QStringLiteral("1e999"), QStringLiteral("-1e999"), QStringLiteral("9223372036854775807"),
        QStringLiteral("99999999999999999999"), QStringLiteral("0x7fffffff"), QStringLiteral("true"), QStringLiteral("false"),
        QStringLiteral("null"), QStringLiteral("\"\""), QStringLiteral("\"off\""), QStringLiteral("\"#zz\""), QStringLiteral("\"(\""),
        QStringLiteral("\"שלום\""), QStringLiteral("r#\"[\"#"), QStringLiteral("{ }"), QStringLiteral("x=1")};
    return values;
}

QString mutate(QString text, QRandomGenerator &random)
{
    static const QRegularExpression value(QStringLiteral("-?\\d[\\w.+-]*|\"(?:[^\"\\\\\\n]|\\\\.)*\"|\\btrue\\b|\\bfalse\\b"));
    QStringList lines = text.split(u'\n');
    const int steps = 1 + random.bounded(3);
    for (int step = 0; step < steps; ++step) {
        const auto line = static_cast<qsizetype>(random.bounded(static_cast<int>(lines.size())));
        switch (random.bounded(5)) {
        case 0:
            lines.removeAt(line);
            break;
        case 1:
            lines.insert(line, lines.at(line));
            break;
        case 2:
            lines.swapItemsAt(line, random.bounded(static_cast<int>(lines.size())));
            break;
        case 3:
            lines[line].remove(QRegularExpression(QStringLiteral("^\\s*//\\s?")));
            break;
        default: {
            QList<QRegularExpressionMatch> matches;
            for (auto iterator = value.globalMatch(lines.at(line)); iterator.hasNext();) {
                matches.append(iterator.next());
            }
            if (!matches.isEmpty()) {
                const QRegularExpressionMatch &match = matches.at(random.bounded(static_cast<int>(matches.size())));
                const QStringList &values = replacementValues();
                lines[line].replace(
                    match.capturedStart(), match.capturedLength(), values.at(random.bounded(static_cast<int>(values.size()))));
            }
        }
        }
        if (lines.isEmpty()) {
            break;
        }
    }
    return lines.join(u'\n');
}

}

class TestConfigFuzz : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void brokenDefaultConfigsLoadOrPointAtTheirMistake();
};

void TestConfigFuzz::brokenDefaultConfigsLoadOrPointAtTheirMistake()
{
    const QString original = readFile(QStringLiteral(KONVEYOR_SOURCE_DIR "/data/default-config.kdl"));
    QVERIFY(!original.isEmpty());
    QRandomGenerator random(7);
    int failures = 0;
    for (int iteration = 0; iteration < Iterations; ++iteration) {
        const QString text = mutate(original, random);
        const auto result = loadString(text, QStringLiteral("config.kdl"));
        if (result) {
            continue;
        }
        ++failures;
        const LoadError &error = result.error();
        const QStringList lines = text.split(u'\n');
        const QString context = QStringLiteral("iteration %1: %2").arg(iteration).arg(error.toString());
        QVERIFY2(!error.message.isEmpty(), qPrintable(context));
        QVERIFY2(error.location.file == QStringLiteral("config.kdl"), qPrintable(context));
        QVERIFY2(error.location.line >= 1 && error.location.line <= lines.size(), qPrintable(context));
        QVERIFY2(error.location.column >= 1, qPrintable(context));
        QVERIFY2(error.sourceLine == lines.at(error.location.line - 1), qPrintable(context));
    }
    QVERIFY(failures > Iterations / 4);
}

QTEST_GUILESS_MAIN(TestConfigFuzz)

#include "test_config_fuzz.moc"
