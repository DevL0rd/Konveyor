#include "../dbus/fakekglobalaccel.h"
#include "shortcutconflicts.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QTest>

#include <memory>

using Konveyor::ReleasedShortcut;
using Konveyor::ShortcutConflicts;
using Konveyor::Test::FakeKGlobalAccel;
using Konveyor::Test::PrivateSession;

namespace
{

const QStringList overview {QStringLiteral("kwin"), QStringLiteral("Overview"), QStringLiteral("KWin"), QStringLiteral("Toggle Overview")};
const QKeySequence metaW(QStringLiteral("Meta+W"));

}

class TestShortcutConflicts : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        m_session = std::make_unique<PrivateSession>();
        QVERIFY(m_session->start());
        m_fake = std::make_unique<FakeKGlobalAccel>();
        QVERIFY(m_fake->start(m_session->address()));
        m_fake->add(overview, {metaW});
        m_fake->add({QStringLiteral("konveyor"), QStringLiteral("focus-column-left"), QStringLiteral("Konveyor"), QString()},
            {QKeySequence(QStringLiteral("Meta+H"))});
    }

    void cleanupTestCase()
    {
        m_fake.reset();
        m_session.reset();
    }

    void takeOverStealsAForeignKeyAndRemembersIt()
    {
        const QList<ReleasedShortcut> released = ShortcutConflicts::takeOver({metaW}, QStringLiteral("konveyor"));
        QCOMPARE(released.size(), 1);
        QCOMPARE(released.first().component, QStringLiteral("kwin"));
        QCOMPARE(released.first().action, QStringLiteral("Overview"));
        QCOMPARE(released.first().componentFriendlyName, QStringLiteral("KWin"));
        QCOMPARE(released.first().actionFriendlyName, QStringLiteral("Toggle Overview"));
        QCOMPARE(released.first().keys, QList<QKeySequence> {metaW});
        FakeKGlobalAccel::settle();
        QVERIFY(!m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Overview")).contains(metaW));

        QVERIFY(ShortcutConflicts::restore(released).isEmpty());
        QCOMPARE(m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Overview")), QList<QKeySequence> {metaW});
    }

    void takeOverLeavesItsOwnAndFreeKeysAlone()
    {
        FakeKGlobalAccel::settle();
        const auto before = m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size();
        const QList<ReleasedShortcut> released = ShortcutConflicts::takeOver(
            {QKeySequence(QStringLiteral("Meta+H")), QKeySequence(QStringLiteral("Meta+Y"))}, QStringLiteral("konveyor"));
        FakeKGlobalAccel::settle();
        QVERIFY(released.isEmpty());
        QCOMPARE(m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size(), before);
        QCOMPARE(m_fake->keys(QStringLiteral("konveyor"), QStringLiteral("focus-column-left")),
            QList<QKeySequence> {QKeySequence(QStringLiteral("Meta+H"))});
    }

    void takeOverLeavesAKeyTheUserGaveToAnotherAction()
    {
        const QKeySequence metaR(QStringLiteral("Meta+R"));
        const QStringList reboot {QStringLiteral("ksmserver"), QStringLiteral("Reboot Without Confirmation"),
            QStringLiteral("Session Management"), QStringLiteral("Reboot Without Confirmation")};
        m_fake->add(reboot, {metaR}, {});
        m_fake->add({QStringLiteral("org.kde.spectacle.desktop"), QStringLiteral("RecordRegion"), QStringLiteral("Spectacle"),
                        QStringLiteral("Record Region")},
            {metaR});
        FakeKGlobalAccel::settle();
        const auto before = m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size();
        QCOMPARE(ShortcutConflicts::userHeld({metaR, metaW, QKeySequence(QStringLiteral("Meta+Y"))}, QStringLiteral("konveyor")),
            QList<QKeySequence> {metaR});
        QVERIFY(ShortcutConflicts::takeOver({metaR}, QStringLiteral("konveyor")).isEmpty());
        FakeKGlobalAccel::settle();
        QCOMPARE(m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size(), before);
        QCOMPARE(m_fake->keys(QStringLiteral("ksmserver"), QStringLiteral("Reboot Without Confirmation")), QList<QKeySequence> {metaR});
        QCOMPARE(m_fake->keys(QStringLiteral("org.kde.spectacle.desktop"), QStringLiteral("RecordRegion")), QList<QKeySequence> {metaR});
    }

    void releaseSupersededFreesEditTiles()
    {
        QVERIFY(ShortcutConflicts::releaseSuperseded().isEmpty());
        const QKeySequence metaT(QStringLiteral("Meta+T"));
        m_fake->add(
            {QStringLiteral("kwin"), QStringLiteral("Edit Tiles"), QStringLiteral("KWin"), QStringLiteral("Toggle Tiles Editor")}, {metaT});
        const QList<ReleasedShortcut> released = ShortcutConflicts::releaseSuperseded();
        QCOMPARE(released.size(), 1);
        QCOMPARE(released.first().action, QStringLiteral("Edit Tiles"));
        QCOMPARE(released.first().keys, QList<QKeySequence> {metaT});
        FakeKGlobalAccel::settle();
        QVERIFY(!m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Edit Tiles")).contains(metaT));
        QVERIFY(ShortcutConflicts::restore(released).isEmpty());
        QCOMPARE(m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Edit Tiles")), QList<QKeySequence> {metaT});
    }

    void restoreSendsTheFriendlyNames()
    {
        const ReleasedShortcut shortcut {QStringLiteral("plasmashell"), QStringLiteral("show dashboard"), QStringLiteral("Plasma"),
            QStringLiteral("Show Desktop"), {QKeySequence(QStringLiteral("Ctrl+F12"))}};
        QVERIFY(ShortcutConflicts::restore({shortcut}).isEmpty());
        const QDBusMessage call = m_fake->calls(QStringLiteral("setForeignShortcutKeys")).last();
        QCOMPARE(call.arguments().value(0).toStringList(),
            QStringList({QStringLiteral("plasmashell"), QStringLiteral("show dashboard"), QStringLiteral("Plasma"),
                QStringLiteral("Show Desktop")}));
        QCOMPARE(FakeKGlobalAccel::keysOf(call.arguments().value(1)), shortcut.keys);
    }

    void restoreReportsWhatKGlobalAccelRefused()
    {
        const ReleasedShortcut shortcut {QStringLiteral("kwin"), QStringLiteral("Overview"), QStringLiteral("KWin"), QString(), {metaW}};
        m_fake.reset();
        const QList<ReleasedShortcut> failed = ShortcutConflicts::restore({shortcut});
        m_fake = std::make_unique<FakeKGlobalAccel>();
        QVERIFY(m_fake->start(m_session->address()));
        QCOMPARE(failed.size(), 1);
        QCOMPARE(failed.first().action, QStringLiteral("Overview"));
    }

    void restoreLeavesAnActionOnTheKeysItWasGivenSince()
    {
        const QKeySequence metaP(QStringLiteral("Meta+P"));
        m_fake->add(
            {QStringLiteral("kwin"), QStringLiteral("Present Windows"), QStringLiteral("KWin"), QStringLiteral("Present")}, {metaP});
        const ReleasedShortcut shortcut {QStringLiteral("kwin"), QStringLiteral("Present Windows"), QStringLiteral("KWin"),
            QStringLiteral("Present"), {QKeySequence(QStringLiteral("Meta+Q"))}};
        FakeKGlobalAccel::settle();
        const auto before = m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size();
        QVERIFY(ShortcutConflicts::restore({shortcut}).isEmpty());
        QCOMPARE(m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size(), before);
        QCOMPARE(m_fake->keys(QStringLiteral("kwin"), QStringLiteral("Present Windows")), QList<QKeySequence> {metaP});
    }

    void restoreUnwantedGivesBackOnlyWhatIsNoLongerWanted()
    {
        const ReleasedShortcut wanted {QStringLiteral("kwin"), QStringLiteral("Overview"), QStringLiteral("KWin"), QString(), {metaW}};
        const ReleasedShortcut unwanted {QStringLiteral("plasmashell"), QStringLiteral("show dashboard"), QStringLiteral("Plasma"),
            QStringLiteral("Show Desktop"), {QKeySequence(QStringLiteral("Ctrl+F12"))}};
        FakeKGlobalAccel::settle();
        const auto before = m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size();
        const QList<ReleasedShortcut> kept = ShortcutConflicts::restoreUnwanted({wanted, unwanted}, {metaW});
        QCOMPARE(kept.size(), 1);
        QCOMPARE(kept.first().action, QStringLiteral("Overview"));
        QCOMPARE(m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size(), before + 1);
    }

    void restoreUnwantedKeepsTheSupersededActionsWhateverIsWanted()
    {
        const ReleasedShortcut tiles {QStringLiteral("kwin"), QStringLiteral("Edit Tiles"), QStringLiteral("KWin"), QString(),
            {QKeySequence(QStringLiteral("Meta+T"))}};
        FakeKGlobalAccel::settle();
        const auto before = m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size();
        const QList<ReleasedShortcut> kept = ShortcutConflicts::restoreUnwanted({tiles}, {});
        QCOMPARE(kept.size(), 1);
        QCOMPARE(kept.first().action, QStringLiteral("Edit Tiles"));
        QCOMPARE(m_fake->calls(QStringLiteral("setForeignShortcutKeys")).size(), before);
    }

    void restoreUnwantedKeepsWhatKGlobalAccelRefused()
    {
        const ReleasedShortcut unwanted {QStringLiteral("plasmashell"), QStringLiteral("show dashboard"), QStringLiteral("Plasma"),
            QStringLiteral("Show Desktop"), {QKeySequence(QStringLiteral("Ctrl+F12"))}};
        m_fake.reset();
        const QList<ReleasedShortcut> kept = ShortcutConflicts::restoreUnwanted({unwanted}, {});
        m_fake = std::make_unique<FakeKGlobalAccel>();
        QVERIFY(m_fake->start(m_session->address()));
        QCOMPARE(kept.size(), 1);
        QCOMPARE(kept.first().action, QStringLiteral("show dashboard"));
    }

    void saveAndLoadRoundTrip()
    {
        const QList<ReleasedShortcut> released {
            {QStringLiteral("kwin"), QStringLiteral("Overview"), QStringLiteral("KWin"), QStringLiteral("Toggle Overview"),
                {metaW, QKeySequence(QStringLiteral("Ctrl+Alt+Tab"))}},
            {QStringLiteral("plasmashell"), QStringLiteral("activate task manager entry 1"), QStringLiteral("Plasma"), QString(), {}},
        };
        ShortcutConflicts::save(released);
        const QList<ReleasedShortcut> loaded = ShortcutConflicts::load();
        QCOMPARE(loaded.size(), 2);
        for (const ReleasedShortcut &expected : released) {
            const auto found = std::ranges::find_if(loaded, [&](const ReleasedShortcut &entry) { return entry.action == expected.action; });
            QVERIFY(found != loaded.end());
            QCOMPARE(found->component, expected.component);
            QCOMPARE(found->componentFriendlyName, expected.componentFriendlyName);
            QCOMPARE(found->actionFriendlyName, expected.actionFriendlyName);
            QCOMPARE(found->keys, expected.keys);
        }
        QVERIFY(QFile::exists(m_session->dir(QStringLiteral("state")) + QStringLiteral("/konveyorstaterc")));

        ShortcutConflicts::save({});
        QVERIFY(ShortcutConflicts::load().isEmpty());
    }

    void mergeSkipsKnownActions()
    {
        QList<ReleasedShortcut> into {{QStringLiteral("kwin"), QStringLiteral("Overview"), {}, {}, {metaW}}};
        ShortcutConflicts::merge(into,
            {{QStringLiteral("kwin"), QStringLiteral("Overview"), {}, {}, {}},
                {QStringLiteral("kwin"), QStringLiteral("Grid View"), {}, {}, {}},
                {QStringLiteral("plasmashell"), QStringLiteral("Overview"), {}, {}, {}}});
        QCOMPARE(into.size(), 3);
        QCOMPARE(into.first().keys, QList<QKeySequence> {metaW});
    }

private:
    std::unique_ptr<PrivateSession> m_session;
    std::unique_ptr<FakeKGlobalAccel> m_fake;
};

QTEST_GUILESS_MAIN(TestShortcutConflicts)
#include "test_shortcut_conflicts.moc"
