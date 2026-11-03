#include "preview/borderstylematerial.h"

#include <QTest>

using namespace Konveyor::Settings;

class TestBorderStyleMaterial : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void everyShaderKindHasItsOwnMaterialType()
    {
        const BorderStyleMaterial band(BorderStyleShaderKind::Band);
        const BorderStyleMaterial otherBand(BorderStyleShaderKind::Band);
        const BorderStyleMaterial particle(BorderStyleShaderKind::Particle);
        const BorderStyleMaterial reactive(BorderStyleShaderKind::Reactive);
        QCOMPARE(band.type(), otherBand.type());
        QVERIFY(band.type() != particle.type());
        QVERIFY(band.type() != reactive.type());
        QVERIFY(particle.type() != reactive.type());
        QCOMPARE(reactive.shader(), BorderStyleShaderKind::Reactive);
    }

    void materialsAreOnlyEqualToThemselves()
    {
        const BorderStyleMaterial first(BorderStyleShaderKind::Band);
        const BorderStyleMaterial second(BorderStyleShaderKind::Band);
        QCOMPARE(first.compare(&first), 0);
        QVERIFY(first.compare(&second) != 0);
        QCOMPARE(first.compare(&second), -second.compare(&first));
    }
};

QTEST_GUILESS_MAIN(TestBorderStyleMaterial)

#include "test_border_style_material.moc"
