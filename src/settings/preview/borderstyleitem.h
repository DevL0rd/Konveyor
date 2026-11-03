#pragma once

#include "preview/borderstylelayers.h"
#include "render/drive.h"
#include "render/eventlayer.h"

#include <QList>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>
#include <QVector4D>
#include <qqmlregistration.h>

#include <memory>
#include <vector>

namespace Konveyor::Settings
{

class BorderStyleClock;

struct LayerFrame
{
    double pixels = 1.0;
    double phase = 0.0;
    double seconds = 0.0;
    float opacity = 1.f;
    bool hidden = false;
    QVector4D params;
    QVector4D front;
    const Render::LayerMotion *motion = nullptr;
};

class BorderStyleItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList layers MEMBER m_layerValues NOTIFY layersChanged)
    Q_PROPERTY(QVariantMap theme MEMBER m_theme NOTIFY layersChanged)
    Q_PROPERTY(QVector4D radii MEMBER m_radii NOTIFY appearanceChanged)
    Q_PROPERTY(double baseWidth MEMBER m_baseWidth NOTIFY appearanceChanged)
    Q_PROPERTY(double inset MEMBER m_inset NOTIFY appearanceChanged)
    Q_PROPERTY(double zoom MEMBER m_zoom NOTIFY appearanceChanged)
    Q_PROPERTY(int emphasis MEMBER m_emphasis NOTIFY appearanceChanged)
    Q_PROPERTY(double rippleRest MEMBER m_rippleRest NOTIFY appearanceChanged)
    Q_PROPERTY(double eventTime MEMBER m_eventTime NOTIFY appearanceChanged)
    Q_PROPERTY(QRectF eventFrame MEMBER m_eventFrame NOTIFY appearanceChanged)
    Q_PROPERTY(QRectF eventSubject MEMBER m_eventSubject NOTIFY appearanceChanged)
    Q_PROPERTY(QRectF eventFrom MEMBER m_eventFrom NOTIFY appearanceChanged)
    Q_PROPERTY(QRectF eventTo MEMBER m_eventTo NOTIFY appearanceChanged)
    Q_PROPERTY(double eventFarthest MEMBER m_eventFarthest NOTIFY appearanceChanged)
    Q_PROPERTY(double eventLength MEMBER m_eventLength NOTIFY appearanceChanged)
    Q_PROPERTY(bool reduceMotion MEMBER m_reduceMotion NOTIFY appearanceChanged)

public:
    explicit BorderStyleItem(QQuickItem *parent = nullptr);
    ~BorderStyleItem() override;

    void advance();
    Q_INVOKABLE double eventDuration() const;

Q_SIGNALS:
    void layersChanged();
    void appearanceChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &value) override;

private:
    void parseLayers();
    void updateAnimating();
    bool reachesViewport();
    double outerReach() const;
    Render::EventScene eventScene() const;
    LayerFrame frameFor(int index, double ratio, double now, const QSizeF &frameSize);
    void applyOneShot(
        LayerFrame &frame, const Config::BorderLayer &config, const Render::LayerMotion &motion, double now, const QSizeF &frameSize) const;

    std::shared_ptr<BorderStyleClock> m_clock;
    QVariantList m_layerValues;
    QVariantMap m_theme;
    QList<PreviewLayer> m_parsed;
    std::vector<Render::LayerMotion> m_motions;
    QVector4D m_radii;
    double m_baseWidth = 4.0;
    double m_inset = 0.0;
    double m_zoom = 1.0;
    double m_rippleRest = 3.0;
    double m_eventTime = -1.0;
    QRectF m_eventFrame;
    QRectF m_eventSubject;
    QRectF m_eventFrom;
    QRectF m_eventTo;
    double m_eventFarthest = 0.0;
    double m_eventLength = 0.0;
    int m_emphasis = -1;
    bool m_reduceMotion = false;
};

}
