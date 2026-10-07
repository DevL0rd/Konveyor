#include "input/heldmodifiers.h"

#include <QStringList>
#include <QTest>

using Konveyor::HeldModifiers;

class TestHeldModifiers : public QObject
{
    Q_OBJECT

private:
    QStringList m_changes;
    HeldModifiers m_held {[this](bool super, bool alt) { m_changes.append(QStringLiteral("%1 %2").arg(super).arg(alt)); }};

private Q_SLOTS:
    void init()
    {
        m_held.clear();
        m_changes.clear();
    }

    void metaThenMetaAltThenRelease()
    {
        m_held.key(Qt::Key_Meta, true, Qt::NoModifier);
        m_held.key(Qt::Key_Alt, true, Qt::MetaModifier);
        m_held.key(Qt::Key_Alt, false, Qt::MetaModifier | Qt::AltModifier);
        m_held.key(Qt::Key_Meta, false, Qt::MetaModifier);
        QCOMPARE(m_changes, (QStringList {QStringLiteral("1 0"), QStringLiteral("1 1"), QStringLiteral("1 0"), QStringLiteral("0 0")}));
    }

    void aMissedMetaReleaseIsCorrectedByTheNextKey()
    {
        m_held.key(Qt::Key_Meta, true, Qt::NoModifier);
        m_held.key(Qt::Key_L, true, Qt::MetaModifier);
        m_held.key(Qt::Key_A, true, Qt::NoModifier);
        QCOMPARE(m_changes, (QStringList {QStringLiteral("1 0"), QStringLiteral("0 0")}));
    }

    void aMissedReleaseIsCorrectedByThePointer()
    {
        m_held.key(Qt::Key_Meta, true, Qt::NoModifier);
        m_held.key(Qt::Key_Alt, true, Qt::MetaModifier);
        m_held.sync(Qt::NoModifier);
        QCOMPARE(m_changes, (QStringList {QStringLiteral("1 0"), QStringLiteral("1 1"), QStringLiteral("0 0")}));
    }

    void heldModifiersSurviveThePointer()
    {
        m_held.key(Qt::Key_Meta, true, Qt::NoModifier);
        m_held.sync(Qt::MetaModifier);
        QCOMPARE(m_changes, (QStringList {QStringLiteral("1 0")}));
    }

    void lockingTheScreenClearsHeldModifiers()
    {
        m_held.key(Qt::Key_Meta, true, Qt::NoModifier);
        m_held.clear();
        QCOMPARE(m_changes, (QStringList {QStringLiteral("1 0"), QStringLiteral("0 0")}));
    }
};

QTEST_GUILESS_MAIN(TestHeldModifiers)

#include "test_held_modifiers.moc"
