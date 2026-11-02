#pragma once

#include "config/types/paint.h"
#include "config/types/signal.h"

#include <QList>
#include <QString>

#include <utility>

namespace Konveyor::Config
{

inline constexpr int kBorderLayerMaxColors = 8;
inline constexpr int kBorderLayerMaxParticles = 256;
inline constexpr int kBorderLayerMaxCount = 64;

enum class BorderLayerKind
{
    Gradient,
    Sweep,
    Comet,
    Glow,
    Ripple,
    Dashes,
    Pulse,
    Particles,
    Equalizer,
    Waveform,
    Spectrum,
    Shimmer,
    BassThump,
    SpeedLines,
    ScrollTrail,
    Bulge,
    Wavy,
    Elastic,
    Chevrons,
    EdgeFlash,
    Gauge,
    PulseFront,
    Wave,
    DrawIn,
    FadeOut
};

enum class BorderBlend
{
    Normal,
    Add,
    Screen,
    Multiply
};

enum class ParticleDirection
{
    Out,
    Up,
    Down,
    Along,
    Random
};

enum class DriveCurve
{
    Linear,
    Square,
    Sqrt,
    Invert
};

struct LayerDrive
{
    QString setting;
    Signal signal = Signal::AudioLevel;
    double amount = 1.0;
    double smooth = 0.0;
    DriveCurve curve = DriveCurve::Linear;
    bool operator==(const LayerDrive &) const = default;
};

struct BorderLayer
{
    BorderLayerKind kind = BorderLayerKind::Gradient;
    QList<Paint> colors;
    double speed = 1.0;
    double width = 0.0;
    double opacity = 1.0;
    BorderBlend blend = BorderBlend::Normal;
    double angle = 0.0;
    int count = 1;
    double tail = 0.25;
    double dash = 12.0;
    double gap = 8.0;
    double rate = 24.0;
    double size = 3.0;
    double lifetime = 1.2;
    ParticleDirection direction = ParticleDirection::Out;
    int burst = 0;
    double thickness = 2.0;
    double sensitivity = 1.0;
    double smoothing = 0.3;
    double baseline = 0.15;
    double position = 0.0;
    QList<LayerDrive> drives;
    QList<Signal> fireOn;
    bool operator==(const BorderLayer &) const = default;
};

const QList<std::pair<QString, BorderLayerKind>> &borderLayerKinds();

QStringList drivableSettings();
void addToDrivenSetting(BorderLayer &layer, const QString &setting, double amount);
void applyKindDefaults(BorderLayer &layer);
QStringList driveCurveNames();

struct BorderStyle
{
    QString name;
    QList<BorderLayer> layers;
    bool operator==(const BorderStyle &) const = default;
};

}
