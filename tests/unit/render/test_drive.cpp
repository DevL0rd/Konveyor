#include "config/loader.h"
#include "render/drive.h"

#include <QTest>

using namespace Konveyor;

namespace
{

Config::BorderLayer parsedLayer(const QString &body)
{
    const auto loaded = Config::loadString(QStringLiteral("border-style \"s\" {\n%1\n}\n").arg(body), QStringLiteral("t.kdl"));
    if (!loaded || loaded->config.borderStyles.isEmpty() || loaded->config.borderStyles.first().layers.isEmpty()) {
        return {};
    }
    return loaded->config.borderStyles.first().layers.first();
}

bool parses(const QString &body)
{
    return Config::loadString(QStringLiteral("border-style \"s\" {\n%1\n}\n").arg(body), QStringLiteral("t.kdl")).has_value();
}

}

class TestDrive : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesDriveAndFireOn()
    {
        const Config::BorderLayer layer
            = parsedLayer(QStringLiteral("layer \"particles\" { burst 24; fire-on \"audio.beat\" \"click\"; drive \"rate\" "
                                         "\"scroll.speed\" amount=2 smooth=0.1 curve=\"square\"; }"));
        QCOMPARE(layer.burst, 24);
        QCOMPARE(layer.fireOn, (QList<Config::Signal> {Config::Signal::AudioBeat, Config::Signal::Click}));
        QCOMPARE(layer.drives.size(), 1);
        QCOMPARE(layer.drives.first().setting, QStringLiteral("rate"));
        QCOMPARE(layer.drives.first().curve, Config::DriveCurve::Square);
    }

    void rejectsWhatMakesNoSense()
    {
        QVERIFY(!parses(QStringLiteral("layer \"glow\" { drive \"nope\" \"audio.bass\"; }")));
        QVERIFY(!parses(QStringLiteral("layer \"glow\" { drive \"width\" \"nope\"; }")));
        QVERIFY(!parses(QStringLiteral("layer \"glow\" { drive \"width\"; }")));
        QVERIFY(!parses(QStringLiteral("layer \"particles\" { fire-on \"audio.level\"; }")));
    }

    void kindDefaultsCanBeReplaced()
    {
        const Config::BorderLayer plain = parsedLayer(QStringLiteral("layer \"bass-thump\""));
        QCOMPARE(plain.drives.size(), 1);
        QCOMPARE(plain.drives.first().signal, Config::Signal::AudioBass);
        const Config::BorderLayer changed = parsedLayer(QStringLiteral("layer \"bass-thump\" { drive \"baseline\" \"view.speed\"; }"));
        QCOMPARE(changed.drives.size(), 1);
        QCOMPARE(changed.drives.first().signal, Config::Signal::ViewSpeed);
    }

    void driveAddsToTheBase()
    {
        const Config::BorderLayer layer
            = parsedLayer(QStringLiteral("layer \"glow\" { width 4; drive \"width\" \"audio.bass\" amount=10; }"));
        Render::SignalFrame frame;
        frame.set(Config::Signal::AudioBass, 0.5F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QCOMPARE(motion.resolved.width, 9.0);
        QVERIFY(motion.resolved.drives.isEmpty());
        QCOMPARE(Render::layerOuterReach(layer, 3.0), 15.0);
    }

    void firingMakesOneBurstPerImpulse()
    {
        const Config::BorderLayer layer = parsedLayer(QStringLiteral("layer \"particles\" { count 100; burst 10; fire-on \"click\"; }"));
        QVERIFY(Render::layerBursts(layer));
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QCOMPARE(motion.bursts.number, quint64(0));
        frame.fire(Config::Signal::Click);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        Render::advanceMotion(motion, layer, frame, 1.2, 3.0);
        QCOMPARE(motion.bursts.number, quint64(1));
        const Render::BorderLayerShape shape {QSizeF(100, 100), {}, 0.0, 3.0};
        QCOMPARE(Render::burstsAt(layer, shape, motion.bursts, 1.3).size(), std::size_t(10));
    }

    void busStaysZeroUnlessWanted()
    {
        Render::SignalBus bus;
        bus.set(1, Config::Signal::ScrollSpeed, 0.7F);
        QCOMPARE(bus.frame(1).value(Config::Signal::ScrollSpeed), 0.0F);
        Config::SignalSet wanted;
        wanted.set(static_cast<std::size_t>(Config::Signal::ScrollSpeed));
        bus.setWanted(Render::SignalClient::Borders, 1, wanted);
        bus.set(1, Config::Signal::ScrollSpeed, 0.7F);
        QCOMPARE(bus.frame(1).value(Config::Signal::ScrollSpeed), 0.7F);
        QVERIFY(bus.wantedAnywhere().any());
        bus.setWanted(Render::SignalClient::Borders, 1, {});
        QVERIFY(bus.wantedAnywhere().none());
        QCOMPARE(bus.frame(1).value(Config::Signal::ScrollSpeed), 0.0F);
    }

    void inputLayersHideUntilTheirSourceMovesAndFadeWhenItStops()
    {
        const Config::BorderLayer layer = parsedLayer(QStringLiteral("layer \"bulge\""));
        Render::SignalFrame frame;
        QCOMPARE(Render::layerPresence(layer, frame), 0.0F);
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QCOMPARE(motion.resolved.opacity, 0.0);
        frame.set(Config::Signal::ScrollSpeed, 0.5F);
        QCOMPARE(Render::layerPresence(layer, frame), 1.0F);
        frame.set(Config::Signal::ScrollSpeed, 0.4F);
        frame.fadePresences(0.9);
        QCOMPARE(Render::layerPresence(layer, frame), 1.0F);
        frame.fadePresences(1.0);
        QCOMPARE(Render::layerPresence(layer, frame), 0.0F);
        QCOMPARE(Render::layerPresence(parsedLayer(QStringLiteral("layer \"gradient\"")), frame), 1.0F);
    }

    void busWakesOnlyWhenASilentSourceReturns()
    {
        Render::SignalBus bus;
        Config::SignalSet wanted;
        wanted.set(static_cast<std::size_t>(Config::Signal::ScrollSpeed));
        bus.setWanted(Render::SignalClient::Borders, 1, wanted);
        int woken = 0;
        bus.onSourceReturned([&woken](quint64) { ++woken; });
        bus.set(1, Config::Signal::ScrollSpeed, 0.2F);
        bus.set(1, Config::Signal::ScrollSpeed, 0.5F);
        QCOMPARE(woken, 1);
    }

    void colorMixBlendsWithoutHidingTheLayer()
    {
        const Config::BorderLayer layer = parsedLayer(QStringLiteral(
            "layer \"gradient\" { speed 0; colors \"#000000\" \"#ffffff\"; drive \"color-mix\" \"pointer.speed\" amount=1.0; }"));
        Render::SignalFrame frame;
        Render::LayerMotion motion;
        Render::advanceMotion(motion, layer, frame, 1.0, 3.0);
        QCOMPARE(motion.resolved.opacity, 1.0);
        QCOMPARE(Render::colorMixUniform(&motion), 0.0F);
        QVERIFY(Render::layerRests(layer, frame));
        frame.set(Config::Signal::PointerSpeed, 0.25F);
        Render::advanceMotion(motion, layer, frame, 1.1, 3.0);
        QCOMPARE(Render::colorMixUniform(&motion), 0.25F);
        QVERIFY(!Render::layerRests(layer, frame));
        QCOMPARE(Render::colorMixUniform(nullptr), -1.0F);
    }

    void reactiveShaderLayersNeverRest()
    {
        const Config::BorderLayer layer = parsedLayer(QStringLiteral("layer \"gauge\" { colors \"#000000\" \"#ffffff\"; "
                                                                     "drive \"position\" \"scroll.distance\" amount=0; "
                                                                     "drive \"color-mix\" \"pointer.speed\" amount=1.0; }"));
        QCOMPARE(layer.speed, 0.0);
        QVERIFY(!Render::layerRests(layer, Render::SignalFrame {}));
    }

    void onlyAddLayersKeepTheirHueOverWhatIsBehind()
    {
        QCOMPARE(Render::additiveUniform(parsedLayer(QStringLiteral("layer \"elastic\" { blend \"add\"; }")).blend), 1.0F);
        QCOMPARE(Render::additiveUniform(parsedLayer(QStringLiteral("layer \"elastic\" { blend \"screen\"; }")).blend), 0.0F);
        QCOMPARE(Render::additiveUniform(parsedLayer(QStringLiteral("layer \"elastic\"")).blend), 0.0F);
    }

    void plainLayersCostNothing()
    {
        QVERIFY(!Render::isReactive(parsedLayer(QStringLiteral("layer \"gradient\""))));
        QVERIFY(Render::signalsOf(parsedLayer(QStringLiteral("layer \"gradient\""))).none());
        QVERIFY(Config::anyAudioSignal(Render::signalsOf(parsedLayer(QStringLiteral("layer \"equalizer\"")))));
    }
};

QTEST_GUILESS_MAIN(TestDrive)
#include "test_drive.moc"
