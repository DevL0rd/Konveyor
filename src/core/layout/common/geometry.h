#pragma once

#include "config/types.h"
#include "layout/common/layouttypes.h"
#include "layout/common/options.h"

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

namespace Konveyor::Layout
{

double snapToPixels(double scale, double logical);
double snapToPixelsAtLeastOne(double scale, double logical);
double floorToPixelsAtLeastOne(double scale, double logical);
double ceilToPixels(double scale, double logical);
QPointF roundPoint(QPointF point, double scale);
QSizeF roundSize(QSizeF size, double scale);

int saturatingInt(double value);
int floorToInt(double value);
int roundToInt(double value);
QSize floorSize(QSizeF size);
QSize nonNegativeSize(QSizeF size);
QSizeF clampedNonNegative(QSizeF size);
QSize roundedSize(QSizeF size);

QRectF workAreaWithStruts(QRectF parentArea, double scale, const Config::Struts &struts);
double scrollToReveal(double curX, double viewWidth, double newColX, double newColWidth, double gaps);

int clampToSizeLimits(int x, int minSize, int maxSize);
int clampToSizeLimitsAllowZero(int x, int minSize, int maxSize);

QPointF clampIntoArea(QRectF area, QRectF rect);
QPointF centerInArea(QRectF area, QSizeF size);

qsizetype nearestOutputIndex(QPointF point, const QList<QRectF> &outputs);
QRectF parkedFrame(QRectF frame, QRectF home, const QList<QRectF> &outputs);

PresetExtent measurePreset(const Config::PresetSize &preset, const Options &options, double viewSize, double reservedSize);
PresetExtent measureFloatingPreset(const Config::PresetSize &preset, double viewSize);

double borderExtent(const Config::Border &border);
QSize maxWindowBounds(const Config::Border &border, QSizeF workingAreaSize, QSizeF reservedSize, double gaps);

}
