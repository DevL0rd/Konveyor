#include "appsharness.h"

class TestLauncherSearchPointerQml : public AppsTest::TestCase
{
    Q_OBJECT

private Q_SLOTS:
    void typingHighlightsWhatEnterOpens_data()
    {
        QTest::addColumn<QPoint>("pointer");
        QTest::newRow("pointer away") << QPoint(2, 2);
        for (const int y : {160, 240, 320, 400}) {
            QTest::addRow("pointer resting at %d", y) << QPoint(300, y);
        }
    }

    void typingHighlightsWhatEnterOpens()
    {
        QFETCH(QPoint, pointer);
        QVERIFY(openLibrary(false));
        QTest::mouseMove(m_harness.window(), QPoint(2, 2));
        QTest::mouseMove(m_harness.window(), pointer);
        const QString lit
            = QStringLiteral("launcher.liveSections().map(s => { const out = []; for (let i = 0; i < s.shownCount; ++i) { "
                             "const t = s.itemAtIndex(i); if (t && (t.selected === true || (s.sectionActive && s.currentIndex === i))) "
                             "out.push(i) } return out.join('+') }).join('|')");
        for (const char *term : {"a", "al", "p", "portal", "@a", "@e", "celeste", "e", "kon", "dol", "o"}) {
            eval(QStringLiteral("launcher.setQuery('%1')").arg(QLatin1String(term)));
            TRY_COMPARE(eval(QStringLiteral("launcher.presentedTerm")).toString(), eval(QStringLiteral("launcher.term")).toString());
            QVERIFY2(searchLaidOut(), term);
            const int sections = eval(QStringLiteral("launcher.liveSections().length")).toInt();
            QString expected = QStringLiteral("0");
            for (int section = 1; section < sections; ++section) {
                expected += QLatin1Char('|');
            }
            QVERIFY2(eval(lit).toString() == expected, qPrintable(QStringLiteral("%1: %2").arg(QLatin1String(term), eval(lit).toString())));
            QCOMPARE(eval(QStringLiteral("launcher.currentSection() === launcher.liveSections()[0]")).toBool(), true);
        }
    }

    void searchRowsLightUpUnderThePointer()
    {
        QVERIFY(openLibrary(false));
        for (const char *term :
            {"a", "al", "p", "portal", "@a", "@e", "celeste", "e", "kon", "dol", "fire", "k", "o", "zz", "o", "dolphin", "d"}) {
            eval(QStringLiteral("launcher.setQuery('%1')").arg(QLatin1String(term)));
            TRY_COMPARE(eval(QStringLiteral("launcher.presentedTerm")).toString(), eval(QStringLiteral("launcher.term")).toString());
            QVERIFY2(searchLaidOut(), term);
            const int sections = eval(QStringLiteral("launcher.liveSections().length")).toInt();
            QVERIFY2(sections > 0, term);
            for (int section = 0; section < sections; ++section) {
                const QString grid = QStringLiteral("launcher.liveSections()[%1]").arg(section);
                QCOMPARE(hitsOtherTiles(grid), QString());
                QCOMPARE(staleTiles(grid), QString());
                const int shown = eval(grid + QStringLiteral(".shownCount")).toInt();
                for (int position = 0; position < shown; ++position) {
                    QVERIFY2(hoverLightsOnlyThat(grid, position),
                        qPrintable(
                            QStringLiteral("%1: %2").arg(QLatin1String(term), tileOf(grid, position)->property("label").toString())));
                }
            }
        }
    }
};

LAUNCHER_TEST_MAIN(TestLauncherSearchPointerQml)
#include "test_launcher_search_pointer_qml.moc"
