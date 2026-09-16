#include "render/borderimage.h"

#include "render/colormix.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Konveyor::Render
{

namespace
{

struct CornerCircle
{
    QPointF center;
    double radius = 0;
};

std::optional<CornerCircle> cornerFor(QPointF p, QSizeF size, const Config::CornerRadius &r)
{
    if (p.x() < r.topLeft && p.y() < r.topLeft) {
        return CornerCircle {{r.topLeft, r.topLeft}, r.topLeft};
    }
    if (size.width() - r.topRight < p.x() && p.y() < r.topRight) {
        return CornerCircle {{size.width() - r.topRight, r.topRight}, r.topRight};
    }
    if (size.width() - r.bottomRight < p.x() && size.height() - r.bottomRight < p.y()) {
        return CornerCircle {{size.width() - r.bottomRight, size.height() - r.bottomRight}, r.bottomRight};
    }
    if (p.x() < r.bottomLeft && size.height() - r.bottomLeft < p.y()) {
        return CornerCircle {{r.bottomLeft, size.height() - r.bottomLeft}, r.bottomLeft};
    }
    return std::nullopt;
}

class GradientSampler
{
public:
    explicit GradientSampler(const BorderSpec &spec)
        : m_spec(spec)
    {
        if (!spec.gradient) {
            return;
        }
        const double angle = (spec.gradient->angle - 90.0) * std::numbers::pi / 180.0;
        const QPointF direction(std::cos(angle), std::sin(angle));
        QPointF diagonal(spec.gradientRect.width(), spec.gradientRect.height());
        if (flipsX(direction)) {
            diagonal.setX(-diagonal.x());
        }
        const double length = QPointF::dotProduct(diagonal, direction);
        m_vector = direction * length;
        if (direction.y() < 0) {
            m_vector = -m_vector;
        }
    }

    Rgba sample(QPointF coords) const
    {
        if (!m_spec.gradient) {
            return premultiplied(fromQColor(m_spec.color));
        }
        QPointF p = coords + m_spec.geometryOffset;
        if (flipsX(m_vector)) {
            p.setX(p.x() - m_spec.gradientRect.width());
        }
        const double lengthSquared = QPointF::dotProduct(m_vector, m_vector);
        double fraction = lengthSquared == 0.0 ? 0.0 : QPointF::dotProduct(p, m_vector) / lengthSquared;
        if (m_vector.y() < 0) {
            fraction += 1.0;
        }
        const Config::Gradient &g = *m_spec.gradient;
        return mixColors(fromQColor(g.from), fromQColor(g.to), std::clamp(fraction, 0.0, 1.0), g.interpolation, g.hue);
    }

private:
    static bool flipsX(QPointF v) { return (v.x() < 0 && 0 <= v.y()) || (0 <= v.x() && v.y() < 0); }

    const BorderSpec &m_spec;
    QPointF m_vector;
};

double borderCoverage(QPointF p, const BorderSpec &spec)
{
    double alpha = cornerCoverage(p, spec.size, spec.outerRadius, spec.scale);
    if (spec.borderWidth <= 0) {
        return alpha;
    }
    const QPointF inner = p - QPointF(spec.borderWidth, spec.borderWidth);
    const QSizeF innerSize = spec.size - QSizeF(spec.borderWidth * 2, spec.borderWidth * 2);
    const bool insideInner = inner.x() >= 0 && inner.y() >= 0 && inner.x() <= innerSize.width() && inner.y() <= innerSize.height();
    if (insideInner) {
        alpha *= 1.0 - cornerCoverage(inner, innerSize, shrinkRadius(spec.outerRadius, spec.borderWidth), spec.scale);
    }
    return alpha;
}

bool isInsideBand(int x, int y, int width, int height, int bandPixels)
{
    return x < bandPixels || y < bandPixels || x >= width - bandPixels || y >= height - bandPixels;
}

}

double cornerCoverage(QPointF coords, QSizeF size, const Config::CornerRadius &radius, double scale)
{
    const std::optional<CornerCircle> corner = cornerFor(coords, size, radius);
    if (!corner) {
        return 1.0;
    }
    const double distance = std::hypot(coords.x() - corner->center.x(), coords.y() - corner->center.y());
    const double t = std::clamp((distance - corner->radius) * scale + 0.5, 0.0, 1.0);
    return 1.0 - t * t * (3.0 - 2.0 * t);
}

Config::CornerRadius shrinkRadius(const Config::CornerRadius &radius, double amount)
{
    const auto shrink = [amount](double value) { return std::max(value - amount, 0.0); };
    return {shrink(radius.topLeft), shrink(radius.topRight), shrink(radius.bottomRight), shrink(radius.bottomLeft)};
}

QImage renderBorder(const BorderSpec &spec)
{
    const int width = static_cast<int>(std::ceil(spec.size.width() * spec.scale));
    const int height = static_cast<int>(std::ceil(spec.size.height() * spec.scale));
    QImage image(std::max(width, 1), std::max(height, 1), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    image.setDevicePixelRatio(spec.scale);
    const double maxRadius
        = std::max({spec.outerRadius.topLeft, spec.outerRadius.topRight, spec.outerRadius.bottomRight, spec.outerRadius.bottomLeft});
    const int bandPixels = spec.borderWidth > 0 ? static_cast<int>(std::ceil((std::max(spec.borderWidth, maxRadius) + 1) * spec.scale))
                                                : std::max(width, height);
    const GradientSampler sampler(spec);
    for (int y = 0; y < image.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (!isInsideBand(x, y, width, height, bandPixels)) {
                x = width - bandPixels - 1;
                continue;
            }
            const QPointF p((x + 0.5) / spec.scale, (y + 0.5) / spec.scale);
            const double coverage = borderCoverage(p, spec);
            if (coverage <= 0) {
                continue;
            }
            Rgba color = sampler.sample(p);
            line[x] = toPremultipliedPixel({color.r * coverage, color.g * coverage, color.b * coverage, color.a * coverage});
        }
    }
    return image;
}

}
