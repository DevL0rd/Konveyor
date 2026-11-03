#include "helpers.h"
#include "render/borderlayer.h"

#include <QTest>

#include <algorithm>
#include <cmath>

using namespace Konveyor;
using Kind = Config::BorderLayerKind;
using Render::Testing::layerOf;

namespace
{

const Render::BorderLayerShape kShape {QSizeF(200, 100), {}, 2.0, 6.0};

Config::BorderLayer steadyParticles(Config::ParticleDirection direction)
{
    Config::BorderLayer layer = layerOf(Kind::Particles);
    layer.count = 40;
    layer.lifetime = 1.0;
    layer.rate = 1000.0;
    layer.direction = direction;
    return layer;
}

std::vector<std::pair<Render::BorderParticle, Render::BorderParticle>> movedPairs(const Config::BorderLayer &layer)
{
    const auto before = Render::particlesAt(layer, kShape, 0.30);
    const auto after = Render::particlesAt(layer, kShape, 0.31);
    std::vector<std::pair<Render::BorderParticle, Render::BorderParticle>> pairs;
    for (std::size_t index = 0; index < before.size() && index < after.size(); ++index) {
        if (after[index].ramp > before[index].ramp) {
            pairs.emplace_back(before[index], after[index]);
        }
    }
    return pairs;
}

}

class TestParticles : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everySlotLivesWhenTheRateKeepsUp()
    {
        const auto particles = Render::particlesAt(steadyParticles(Config::ParticleDirection::Out), kShape, 3.7);
        QCOMPARE(particles.size(), std::size_t(40));
    }

    void slowEmittersLeaveSlotsEmpty()
    {
        Config::BorderLayer layer = steadyParticles(Config::ParticleDirection::Out);
        layer.rate = 10.0;
        const auto particles = Render::particlesAt(layer, kShape, 3.7);
        QVERIFY(particles.size() > 5);
        QVERIFY(particles.size() < 35);
    }

    void particlesAreDeterministic()
    {
        const Config::BorderLayer layer = steadyParticles(Config::ParticleDirection::Random);
        const auto first = Render::particlesAt(layer, kShape, 1.25);
        const auto second = Render::particlesAt(layer, kShape, 1.25);
        QCOMPARE(first.size(), second.size());
        for (std::size_t index = 0; index < first.size(); ++index) {
            QCOMPARE(first[index].position, second[index].position);
            QCOMPARE(first[index].alpha, second[index].alpha);
        }
        const auto later = Render::particlesAt(layer, kShape, 1.5);
        QVERIFY(first.front().position != later.front().position);
    }

    void particlesShrinkAndFadeWithAge()
    {
        Config::BorderLayer layer = steadyParticles(Config::ParticleDirection::Out);
        layer.size = 4.0;
        for (const Render::BorderParticle &particle : Render::particlesAt(layer, kShape, 2.9)) {
            QVERIFY(particle.ramp >= 0.0 && particle.ramp < 1.0);
            QVERIFY(particle.alpha >= 0.0 && particle.alpha <= 1.0);
            QVERIFY(std::abs(particle.radius - 4.0 * (1.0 - 0.5 * particle.ramp)) < 1e-9);
            const double fadeIn = std::min(particle.ramp * 8.0, 1.0);
            QVERIFY(std::abs(particle.alpha - fadeIn * (1.0 - particle.ramp)) < 1e-9);
        }
    }

    void upAndDownParticlesRiseAndFall()
    {
        const auto rising = movedPairs(steadyParticles(Config::ParticleDirection::Up));
        const auto falling = movedPairs(steadyParticles(Config::ParticleDirection::Down));
        QVERIFY(rising.size() > 20);
        QVERIFY(falling.size() > 20);
        QVERIFY(std::ranges::all_of(rising, [](const auto &pair) { return pair.second.position.y() < pair.first.position.y(); }));
        QVERIFY(std::ranges::all_of(falling, [](const auto &pair) { return pair.second.position.y() > pair.first.position.y(); }));
    }

    void randomParticlesGoEveryDirection()
    {
        const auto pairs = movedPairs(steadyParticles(Config::ParticleDirection::Random));
        const auto goesLeft
            = std::ranges::count_if(pairs, [](const auto &pair) { return pair.second.position.x() < pair.first.position.x(); });
        const auto goesUp
            = std::ranges::count_if(pairs, [](const auto &pair) { return pair.second.position.y() < pair.first.position.y(); });
        QVERIFY(goesLeft > 5 && goesLeft < static_cast<long>(pairs.size()) - 5);
        QVERIFY(goesUp > 5 && goesUp < static_cast<long>(pairs.size()) - 5);
    }

    void alongParticlesTravelAcrossTheirOutwardHeading()
    {
        const auto along = movedPairs(steadyParticles(Config::ParticleDirection::Along));
        const auto out = movedPairs(steadyParticles(Config::ParticleDirection::Out));
        QVERIFY(along.size() > 20);
        QCOMPARE(along.size(), out.size());
        for (std::size_t index = 0; index < along.size(); ++index) {
            const QPointF a = along[index].second.position - along[index].first.position;
            const QPointF o = out[index].second.position - out[index].first.position;
            const double cosine = QPointF::dotProduct(a, o) / (std::hypot(a.x(), a.y()) * std::hypot(o.x(), o.y()));
            QVERIFY2(std::abs(cosine) < 0.8, qPrintable(QString::number(cosine)));
        }
    }

    void frozenParticlesDoNotTravel()
    {
        Config::BorderLayer layer = steadyParticles(Config::ParticleDirection::Out);
        layer.speed = 0.0;
        const auto pairs = movedPairs(layer);
        QVERIFY(pairs.size() > 20);
        QVERIFY(std::ranges::all_of(pairs, [](const auto &pair) { return pair.first.position == pair.second.position; }));
    }

    void burstsListOnlyLivingBursts()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.count = 100;
        layer.burst = 5;
        layer.lifetime = 1.2;
        Render::LayerBursts bursts;
        bursts.add(1.0);
        bursts.add(2.0);
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 2.5).size(), std::size_t(5));
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 2.1).size(), std::size_t(10));
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 1.5).size(), std::size_t(5));
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 0.5).size(), std::size_t(0));
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 4.0).size(), std::size_t(0));
        QCOMPARE(Render::burstsAt(layer, kShape, {}, 1.0).size(), std::size_t(0));
    }

    void burstsAreCappedByTheCount()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.count = 7;
        layer.burst = 5;
        Render::LayerBursts bursts;
        bursts.add(1.0);
        bursts.add(1.0);
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 1.1).size(), std::size_t(7));
    }

    void burstsWithoutASizeUseTheDefault()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.count = 100;
        layer.burst = 0;
        Render::LayerBursts bursts;
        bursts.add(1.0);
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 1.1).size(), std::size_t(16));
    }

    void onlyTheLastEightBurstsAreKept()
    {
        Config::BorderLayer layer = layerOf(Kind::Particles);
        layer.count = 1000;
        layer.burst = 2;
        layer.lifetime = 5.0;
        Render::LayerBursts bursts;
        for (int index = 1; index <= 10; ++index) {
            bursts.add(index * 0.01);
        }
        QCOMPARE(bursts.number, quint64(10));
        QCOMPARE(Render::burstsAt(layer, kShape, bursts, 0.2).size(), std::size_t(16));
    }

    void bandsCoverTheEdgesAroundTheWindow()
    {
        const Render::BorderLayerShape shape {QSizeF(100, 50), Config::CornerRadius {10, 10, 10, 10}, 0.0, 6.0};
        const auto vertices = Render::bandVertices(shape, 5.0, 2.0);
        QCOMPARE(vertices.size(), std::size_t(24));
        QCOMPARE(vertices[0].position, QVector2D(-10, -10));
        QCOMPARE(vertices[0].attributes, QVector4D(-55, -30, 0, 1));
        QCOMPARE(vertices[2].position, QVector2D(210, 20));
        QCOMPARE(vertices[1].position, QVector2D(210, -10));
        QCOMPARE(vertices[3].position, vertices[2].position);
        QCOMPARE(vertices[5].attributes, vertices[0].attributes);
    }

    void bandsSkipRectanglesWithoutArea()
    {
        const Render::BorderLayerShape rounded {QSizeF(100, 20), Config::CornerRadius {10, 10, 10, 10}, 0.0, 6.0};
        QCOMPARE(Render::bandVertices(rounded, 5.0, 1.0).size(), std::size_t(12));
        const Render::BorderLayerShape bare {QSizeF(100, 20), {}, 0.0, 6.0};
        QCOMPARE(Render::bandVertices(bare, 0.0, 1.0).size(), std::size_t(0));
    }

    void insetPushesTheBandsOutwards()
    {
        const Render::BorderLayerShape shape {QSizeF(100, 50), {}, 3.0, 6.0};
        const auto vertices = Render::bandVertices(shape, 2.0, 1.0);
        QCOMPARE(vertices[0].position, QVector2D(-5, -5));
    }

    void particleQuadsAreCenteredOnTheParticle()
    {
        QVERIFY(Render::particleVertices({}, 1.0).empty());
        const Render::BorderParticle particle {QPointF(10, 20), 3.0, 0.25, 0.5};
        const auto vertices = Render::particleVertices({particle}, 2.0);
        QCOMPARE(vertices.size(), std::size_t(6));
        QCOMPARE(vertices[0].position, QVector2D(14, 34));
        QCOMPARE(vertices[0].attributes, QVector4D(-1, -1, 0.25F, 0.5F));
        QCOMPARE(vertices[2].position, QVector2D(26, 46));
        QCOMPARE(vertices[2].attributes, QVector4D(1, 1, 0.25F, 0.5F));
        QCOMPARE(vertices[5].position, vertices[0].position);
        QCOMPARE(Render::particleVertices({particle, particle}, 1.0).size(), std::size_t(12));
    }
};

QTEST_GUILESS_MAIN(TestParticles)
#include "test_particles.moc"
