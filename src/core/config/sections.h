#pragma once

#include "config/decode.h"
#include "config/types.h"

#include <QHash>
#include <QString>
#include <QStringList>

namespace Konveyor::Config
{

struct LoadContext
{
    Config config;
    QStringList files;
    QStringList warnings;
    QHash<QString, QString> texts;
    QString rootDir;
    QStringList workspaceNames;
    int recursion = 0;
};

BorderRule decodeBorderRule(const Kdl::Node &node);
TabIndicatorPart decodeTabIndicatorPart(const Kdl::Node &node);
InsertHintPart decodeInsertHintPart(const Kdl::Node &node);
Struts decodeStruts(const Kdl::Node &node);
QList<PresetSize> decodePresetSizes(const Kdl::Node &node);
std::optional<PresetSize> decodeDefaultPresetSize(const Kdl::Node &node);
CornerRadius decodeCornerRadius(const Kdl::Node &node);
HotCorners decodeHotCorners(const Kdl::Node &node);

LayoutPart decodeLayoutPart(const Kdl::Node &node, bool enableEmptyBorder);
void mergeBorder(Border &base, const BorderRule &part);

Animations defaultAnimations();
void decodeAnimations(const Kdl::Node &node, Animations &animations);
void decodeGestures(const Kdl::Node &node, Gestures &gestures);
void decodeInput(const Kdl::Node &node, Input &input);
void decodeOutput(LoadContext &context, const Kdl::Node &node);
void decodeMonitorProfile(LoadContext &context, const Kdl::Node &node);
void decodeWorkspace(LoadContext &context, const Kdl::Node &node);

int qtKeyFromKeysym(quint32 keysym);
Qt::KeyboardModifiers qtModifiers(BindModifiers modifiers);

WindowRule decodeWindowRule(const Kdl::Node &node);
void decodeBinds(LoadContext &context, const Kdl::Node &node);
void resolveBinds(Config &config);

void processNode(LoadContext &context, const Kdl::Node &node, const QString &baseDir, const QStringList &stack);
void processDocument(LoadContext &context, const Kdl::Document &document, const QString &baseDir, const QStringList &stack);
void processInclude(LoadContext &context, const Kdl::Node &node, const QString &baseDir, const QStringList &stack);

}
