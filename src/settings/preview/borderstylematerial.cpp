#include "preview/borderstylematerial.h"

#include <QByteArray>
#include <QSGMaterialShader>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace Konveyor::Settings
{

namespace
{

using PipelineState = QSGMaterialShader::GraphicsPipelineState;

struct BlendFactors
{
    PipelineState::BlendFactor source;
    PipelineState::BlendFactor destination;
};

BlendFactors blendFactors(Config::BorderBlend blend)
{
    switch (blend) {
    case Config::BorderBlend::Screen:
        return {PipelineState::One, PipelineState::OneMinusSrcColor};
    case Config::BorderBlend::Multiply:
        return {PipelineState::DstColor, PipelineState::OneMinusSrcAlpha};
    case Config::BorderBlend::Add:
    case Config::BorderBlend::Normal:
        break;
    }
    return {PipelineState::One, PipelineState::OneMinusSrcAlpha};
}

QString fragmentFile(BorderStyleShaderKind shader)
{
    switch (shader) {
    case BorderStyleShaderKind::Particle:
        return QStringLiteral(":/konveyor/borders/particle.frag.qsb");
    case BorderStyleShaderKind::Reactive:
        return QStringLiteral(":/konveyor/borders/reactive.frag.qsb");
    case BorderStyleShaderKind::Band:
        break;
    }
    return QStringLiteral(":/konveyor/borders/band.frag.qsb");
}

class BorderStyleShader : public QSGMaterialShader
{
public:
    explicit BorderStyleShader(BorderStyleShaderKind shader)
    {
        setFlag(UpdatesGraphicsPipelineState);
        setShaderFileName(VertexStage, QStringLiteral(":/konveyor/borders/border.vert.qsb"));
        setShaderFileName(FragmentStage, fragmentFile(shader));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        BorderStyleUniforms uniforms = static_cast<BorderStyleMaterial *>(newMaterial)->uniforms;
        const QMatrix4x4 matrix = state.combinedMatrix();
        std::memcpy(uniforms.matrix.data(), matrix.constData(), sizeof(uniforms.matrix));
        uniforms.opacity = state.opacity();
        QByteArray *buffer = state.uniformData();
        std::memcpy(buffer->data(), &uniforms, std::min(static_cast<std::size_t>(buffer->size()), sizeof(uniforms)));
        return true;
    }

    bool updateGraphicsPipelineState(RenderState &, GraphicsPipelineState *pipeline, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        const BlendFactors factors = blendFactors(static_cast<BorderStyleMaterial *>(newMaterial)->blend);
        pipeline->blendEnable = true;
        pipeline->srcColor = factors.source;
        pipeline->dstColor = factors.destination;
        return true;
    }
};

}

BorderStyleMaterial::BorderStyleMaterial(BorderStyleShaderKind shader)
    : m_shader(shader)
{
    setFlag(Blending | RequiresFullMatrix);
}

QSGMaterialType *BorderStyleMaterial::type() const
{
    static std::array<QSGMaterialType, 3> types;
    return &types[static_cast<std::size_t>(m_shader)];
}

QSGMaterialShader *BorderStyleMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new BorderStyleShader(m_shader);
}

int BorderStyleMaterial::compare(const QSGMaterial *other) const
{
    if (other == this) {
        return 0;
    }
    return other < this ? 1 : -1;
}

}
