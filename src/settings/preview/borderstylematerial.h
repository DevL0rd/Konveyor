#pragma once

#include "config/types.h"

#include <QSGMaterial>

#include <array>
#include <cstdint>

namespace Konveyor::Settings
{

struct BorderStyleUniforms
{
    std::array<float, 16> matrix {};
    std::array<std::array<float, 4>, Config::kBorderLayerMaxColors> colors {};
    std::array<float, 4> radii {};
    std::array<float, 4> params {};
    std::array<float, 4> reactive {};
    std::array<float, 16> audioBands {};
    std::array<float, 64> audioWave {};
    std::array<float, 4> front {};
    std::array<float, 2> halfSize {};
    float opacity = 1.f;
    float layerOpacity = 1.f;
    float pixels = 1.f;
    float inset = 0.f;
    float bandWidth = 0.f;
    float phase = 0.f;
    std::int32_t colorCount = 0;
    std::int32_t kind = 0;
    float colorMix = -1.f;
    float additive = 0.f;
};

static_assert(sizeof(BorderStyleUniforms) == 624);

enum class BorderStyleShaderKind
{
    Band,
    Particle,
    Reactive
};

class BorderStyleMaterial : public QSGMaterial
{
public:
    explicit BorderStyleMaterial(BorderStyleShaderKind shader);

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode mode) const override;
    int compare(const QSGMaterial *other) const override;

    BorderStyleShaderKind shader() const { return m_shader; }

    BorderStyleUniforms uniforms;
    Config::BorderBlend blend = Config::BorderBlend::Normal;

private:
    BorderStyleShaderKind m_shader = BorderStyleShaderKind::Band;
};

}
