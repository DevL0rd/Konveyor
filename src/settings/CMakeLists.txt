find_package(KF6 REQUIRED COMPONENTS CoreAddons Service)
find_package(Qt6 REQUIRED COMPONENTS Quick GuiPrivate DBus)
include(ECMQmlModule)

ecm_add_qml_module(konveyor_settings URI "org.kde.konveyor.settings" GENERATE_PLUGIN_SOURCE NO_CACHEGEN)

target_sources(konveyor_settings PRIVATE
    document/configdocument.cpp
    document/configdocument.h
    document/nodecodec.cpp
    document/nodecodec.h
    document/nodepath.cpp
    document/nodepath.h
    store/configfile.cpp
    store/configfile.h
    store/edithistory.cpp
    store/edithistory.h
    store/settingsnavigation.cpp
    store/settingsnavigation.h
    store/settingsstore.cpp
    store/settingsstore.h
    system/keynames.cpp
    system/keynames.h
    system/livesession.cpp
    system/livesession.h
    values/configvalues.h
    values/globalvalues.cpp
    values/layoutvalues.cpp
    values/livematching.cpp
    values/livematching.h
    values/paintvalues.cpp
)

ecm_target_qml_sources(konveyor_settings SOURCES qml/SettingsHub.qml qml/PageFactory.js)
ecm_target_qml_sources(konveyor_settings PATH catalog SOURCES
    qml/catalog/Actions.js
    qml/catalog/BindKeys.js
    qml/catalog/Kdl.js
    qml/catalog/MotionMath.js
    qml/catalog/Pages.js
    qml/catalog/RuleSummary.js
    qml/catalog/SettingsIndex.js
)
ecm_target_qml_sources(konveyor_settings PATH catalog/index SOURCES
    qml/catalog/index/LayoutIndex.js
    qml/catalog/index/LookIndex.js
    qml/catalog/index/MonitorsIndex.js
    qml/catalog/index/MotionIndex.js
    qml/catalog/index/MouseIndex.js
    qml/catalog/index/PlasmaIndex.js
    qml/catalog/index/RulesIndex.js
    qml/catalog/index/ShortcutsIndex.js
    qml/catalog/index/WorkspacesIndex.js
)
ecm_target_qml_sources(konveyor_settings PATH components SOURCES
    qml/components/ActionArguments.qml
    qml/components/ActionPicker.qml
    qml/components/AngleDial.qml
    qml/components/AnimationCard.qml
    qml/components/ApplicationChooser.qml
    qml/components/AppPicker.qml
    qml/components/BezierEditor.qml
    qml/components/BindEditorDialog.qml
    qml/components/BindOptions.qml
    qml/components/CardHeader.qml
    qml/components/Card.qml
    qml/components/CategoryTile.qml
    qml/components/ChoiceCards.qml
    qml/components/ColorField.qml
    qml/components/CornerRadiusEditor.qml
    qml/components/EdgeScrollCard.qml
    qml/components/GradientOptions.qml
    qml/components/HeroBanner.qml
    qml/components/ModifiedBadge.qml
    qml/components/ModKeyPicker.qml
    qml/components/NamedWorkspaceCard.qml
    qml/components/OutputPicker.qml
    qml/components/PaintEditor.qml
    qml/components/PresetSizeBars.qml
    qml/components/PreviewStage.qml
    qml/components/SearchResults.qml
    qml/components/Segmented.qml
    qml/components/SettingRow.qml
    qml/components/SettingsPage.qml
    qml/components/ShortcutRow.qml
    qml/components/SizePicker.qml
    qml/components/SliderRow.qml
    qml/components/StatusMessages.qml
    qml/components/StrutsEditor.qml
    qml/components/SwitchRow.qml
    qml/components/TabPositionPicker.qml
    qml/components/TriggerEditor.qml
    qml/components/TriState.qml
    qml/components/WorkspaceNameDialog.qml
)
ecm_target_qml_sources(konveyor_settings PATH components/monitors SOURCES
    qml/components/monitors/BoundRow.qml
    qml/components/monitors/ListCard.qml
    qml/components/monitors/MonitorMatchEditor.qml
    qml/components/monitors/MonitorSummary.js
    qml/components/monitors/NamePrompt.qml
    qml/components/monitors/OutputHotCorners.qml
)
ecm_target_qml_sources(konveyor_settings PATH components/rules SOURCES
    qml/components/rules/ColumnPositionRow.qml
    qml/components/rules/FloatingPositionRow.qml
    qml/components/rules/LimitRow.qml
    qml/components/rules/MatchBlockEditor.qml
    qml/components/rules/RuleBorderEditor.qml
    qml/components/rules/RuleCornersField.qml
    qml/components/rules/RuleLookSection.qml
    qml/components/rules/RuleMatchesPreview.qml
    qml/components/rules/RuleOpenSection.qml
    qml/components/rules/RulePaint.js
    qml/components/rules/RuleSizeRow.qml
    qml/components/rules/RuleTriStateRow.qml
    qml/components/rules/StackLimitRow.qml
    qml/components/rules/TextMatchField.qml
)
ecm_target_qml_sources(konveyor_settings PATH pages SOURCES
    qml/pages/LayoutPage.qml
    qml/pages/LookPage.qml
    qml/pages/MonitorProfilePage.qml
    qml/pages/MonitorsPage.qml
    qml/pages/MotionPage.qml
    qml/pages/MousePage.qml
    qml/pages/OutputOverridePage.qml
    qml/pages/PlasmaPage.qml
    qml/pages/RuleEditorPage.qml
    qml/pages/RulesPage.qml
    qml/pages/ShortcutsPage.qml
    qml/pages/WorkspacesPage.qml
)
ecm_target_qml_sources(konveyor_settings PATH previews SOURCES
    qml/previews/AnimationDemo.qml
    qml/previews/CurveGraph.qml
    qml/previews/DecorationPreview.qml
    qml/previews/EdgeZonesMap.qml
    qml/previews/HotCornersMap.qml
    qml/previews/InsertHintPreview.qml
    qml/previews/MockWindow.qml
    qml/previews/ModKeyPreview.qml
    qml/previews/MonitorArrangement.qml
    qml/previews/MonitorFrame.qml
    qml/previews/MotionOverview.qml
    qml/previews/NewColumnDiagram.qml
    qml/previews/PaintFrame.qml
    qml/previews/PlasmaPreview.qml
    qml/previews/StripPreview.qml
    qml/previews/TabIndicatorPreview.qml
    qml/previews/TitlebarDragDemo.qml
    qml/previews/WarpDemo.qml
    qml/previews/WorkspaceStack.qml
)
ecm_target_qml_sources(konveyor_settings PATH sections SOURCES
    qml/sections/BlockSliderRow.qml
    qml/sections/CornerRule.js
    qml/sections/CornersSection.qml
    qml/sections/InsertHintSection.qml
    qml/sections/LayoutKeys.js
    qml/sections/LayoutOverrides.qml
    qml/sections/PlacementSection.qml
    qml/sections/RingSection.qml
    qml/sections/ScopedRow.qml
    qml/sections/ScopedSwitch.qml
    qml/sections/SizesSection.qml
    qml/sections/TabIndicatorSection.qml
    qml/sections/ValueSlider.qml
)

target_include_directories(konveyor_settings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/store ${CMAKE_CURRENT_SOURCE_DIR}/system)
target_compile_options(konveyor_settings PRIVATE -Wall -Wextra -Werror)
target_link_options(konveyor_settings PRIVATE LINKER:--no-undefined)
target_link_libraries(konveyor_settings PRIVATE
    konveyor_config
    konveyor_layout
    KF6::CoreAddons
    KF6::Service
    Qt6::Quick
    Qt6::DBus
    Qt6::GuiPrivate
)

ecm_finalize_qml_module(konveyor_settings DESTINATION ${KDE_INSTALL_QMLDIR})
