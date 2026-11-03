find_package(KF6 REQUIRED COMPONENTS CoreAddons Service Config)
find_package(Qt6 REQUIRED COMPONENTS Quick GuiPrivate DBus ShaderTools)
include(ECMQmlModule)

add_library(konveyor_settings_model STATIC
    document/apprules.cpp
    document/apprules.h
    document/borderdefaults.cpp
    document/borderdefaults.h
    document/configdocument.cpp
    document/configdocument.h
    document/configdocumenttext.cpp
    document/configmigration.cpp
    document/configmigration.h
    document/nodecodec.cpp
    document/nodecodec.h
    document/nodepath.cpp
    document/nodepath.h
    store/configfile.cpp
    store/configfile.h
    store/edithistory.cpp
    store/edithistory.h
    store/includeoverrides.cpp
    store/includeoverrides.h
    values/borderstylevalues.cpp
    values/configvalues.h
    values/globalvalues.cpp
    values/layoutvalues.cpp
    values/livematching.cpp
    values/livematching.h
    values/paintvalues.cpp
)
target_include_directories(konveyor_settings_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(konveyor_settings_model PUBLIC konveyor_config konveyor_layout Qt6::Core Qt6::Gui)
target_compile_options(konveyor_settings_model PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(konveyor_settings_model PROPERTIES POSITION_INDEPENDENT_CODE ON)

ecm_add_qml_module(konveyor_settings URI "org.kde.konveyor.settings" GENERATE_PLUGIN_SOURCE NO_CACHEGEN)

target_sources(konveyor_settings PRIVATE
    preview/borderstyleclock.cpp
    preview/borderstyleclock.h
    preview/borderstyleitem.cpp
    preview/borderstyleitem.h
    preview/borderstylelayers.cpp
    preview/borderstylelayers.h
    preview/borderstylematerial.cpp
    preview/borderstylematerial.h
    store/settingsnavigation.cpp
    store/settingsnavigation.h
    store/settingsstore.cpp
    store/settingsstore.h
    store/taskbarsettings.cpp
    store/taskbarsettings.h
    system/keynames.cpp
    system/keynames.h
    system/livesession.cpp
    system/livesession.h
)

qt_add_resources(konveyor_settings taskbar_schema PREFIX /konveyor BASE store FILES store/taskbar.kcfg)

set(preview_preludes "${CMAKE_CURRENT_SOURCE_DIR}/preview/shaders")
set(preview_shaders "${CMAKE_CURRENT_BINARY_DIR}/preview-shaders")
konveyor_compose_shader(preview_vertex "${preview_preludes}/vertex.prelude.glsl" "${preview_preludes}/uniforms.glsl" "${KONVEYOR_BORDER_SHADERS}/border.vert.glsl")
konveyor_compose_shader(preview_band "${preview_preludes}/fragment.prelude.glsl" "${preview_preludes}/uniforms.glsl" "${preview_preludes}/finish.glsl" "${KONVEYOR_BORDER_SHADERS}/frame.glsl" "${KONVEYOR_BORDER_SHADERS}/perimeter.glsl" "${KONVEYOR_BORDER_SHADERS}/band.frag.glsl")
konveyor_compose_shader(preview_reactive "${preview_preludes}/fragment.prelude.glsl" "${preview_preludes}/uniforms.glsl" "${preview_preludes}/finish.glsl" "${KONVEYOR_BORDER_SHADERS}/frame.glsl" "${KONVEYOR_BORDER_SHADERS}/perimeter.glsl" "${KONVEYOR_BORDER_SHADERS}/reactive.frag.glsl")
konveyor_compose_shader(preview_particle "${preview_preludes}/fragment.prelude.glsl" "${preview_preludes}/uniforms.glsl" "${preview_preludes}/finish.glsl" "${KONVEYOR_BORDER_SHADERS}/frame.glsl" "${KONVEYOR_BORDER_SHADERS}/particle.frag.glsl")
file(CONFIGURE OUTPUT "${preview_shaders}/border.vert" @ONLY CONTENT "${preview_vertex}")
file(CONFIGURE OUTPUT "${preview_shaders}/band.frag" @ONLY CONTENT "${preview_band}")
file(CONFIGURE OUTPUT "${preview_shaders}/reactive.frag" @ONLY CONTENT "${preview_reactive}")
file(CONFIGURE OUTPUT "${preview_shaders}/particle.frag" @ONLY CONTENT "${preview_particle}")
qt_add_shaders(konveyor_settings "border_style_shaders"
    BATCHABLE
    GLSL "300es,330"
    NOHLSL
    NOMSL
    PREFIX "/konveyor/borders"
    BASE "${preview_shaders}"
    FILES "${preview_shaders}/border.vert" "${preview_shaders}/band.frag" "${preview_shaders}/reactive.frag" "${preview_shaders}/particle.frag"
)

ecm_target_qml_sources(konveyor_settings SOURCES qml/PageFactory.js)
ecm_target_qml_sources(konveyor_settings PATH catalog SOURCES
    qml/catalog/Actions.js
    qml/catalog/BorderActions.js
    qml/catalog/BorderPresets.js
    qml/catalog/BorderStyles.js
    qml/catalog/EventEffects.js
    qml/catalog/BindKeys.js
    qml/catalog/Kdl.js
    qml/catalog/Signals.js
    qml/catalog/MotionMath.js
    qml/catalog/Pages.js
    qml/catalog/RuleSummary.js
    qml/catalog/SettingsIndex.js
)
ecm_target_qml_sources(konveyor_settings PATH catalog/index SOURCES
    qml/catalog/index/AppearanceIndex.js
    qml/catalog/index/ExperimentsIndex.js
    qml/catalog/index/LayoutIndex.js
    qml/catalog/index/MonitorsIndex.js
    qml/catalog/index/MotionIndex.js
    qml/catalog/index/MouseIndex.js
    qml/catalog/index/TouchIndex.js
    qml/catalog/index/PlasmaIndex.js
    qml/catalog/index/TaskbarIndex.js
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
    qml/components/ChoiceCards.qml
    qml/components/ColorField.qml
    qml/components/ColorPickerPopup.qml
    qml/components/ColorSwatch.qml
    qml/components/CornerRadiusEditor.qml
    qml/components/EdgeScrollCard.qml
    qml/components/GradientOptions.qml
    qml/components/HeaderLookup.js
    qml/components/ModifiedBadge.qml
    qml/components/ModKeyPicker.qml
    qml/components/NamedWorkspaceCard.qml
    qml/components/OutputPicker.qml
    qml/components/PaintEditor.qml
    qml/components/PresetSizeBars.qml
    qml/components/PreviewStage.qml
    qml/components/Segmented.qml
    qml/components/SettingRow.qml
    qml/components/LaterSection.qml
    qml/components/SectionJumpBar.qml
    qml/components/SettingsPage.qml
    qml/components/SettingsRail.qml
    qml/components/SettingsStack.qml
    qml/components/SettingsView.qml
    qml/components/ShortcutRow.qml
    qml/components/SizePicker.qml
    qml/components/SliderRow.qml
    qml/components/StatusMessages.qml
    qml/components/StyleChooser.qml
    qml/components/StyleSwatchButton.qml
    qml/components/StrutsEditor.qml
    qml/components/SwitchRow.qml
    qml/components/TaskbarChoiceRow.qml
    qml/components/TaskbarSliderRow.qml
    qml/components/TaskbarSwitchRow.qml
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
    qml/components/rules/RuleAppearanceSection.qml
    qml/components/rules/RuleMatchesPreview.qml
    qml/components/rules/RuleOpenSection.qml
    qml/components/rules/RulePaint.js
    qml/components/rules/RuleSizeRow.qml
    qml/components/rules/RuleTriStateRow.qml
    qml/components/rules/StackLimitRow.qml
    qml/components/rules/TextMatchField.qml
)
ecm_target_qml_sources(konveyor_settings PATH pages SOURCES
    qml/pages/AppearancePage.qml
    qml/pages/BorderStyleEditorPage.qml
    qml/pages/ExperimentsPage.qml
    qml/pages/LayoutPage.qml
    qml/pages/MonitorProfilePage.qml
    qml/pages/MonitorsPage.qml
    qml/pages/MotionPage.qml
    qml/pages/MousePage.qml
    qml/pages/OutputOverridePage.qml
    qml/pages/PlasmaPage.qml
    qml/pages/RuleEditorPage.qml
    qml/pages/RulesPage.qml
    qml/pages/TaskbarPage.qml
    qml/pages/ShortcutsPage.qml
    qml/pages/TouchPage.qml
    qml/pages/WorkspacesPage.qml
)
ecm_target_qml_sources(konveyor_settings PATH previews SOURCES
    qml/previews/AnimationDemo.qml
    qml/previews/BorderStylePreview.qml
    qml/previews/ChoiceDiagram.qml
    qml/previews/CurveGraph.qml
    qml/previews/DecorationPreview.qml
    qml/previews/EdgeZonesMap.qml
    qml/previews/EventEffectScene.qml
    qml/previews/EventPlayClock.qml
    qml/previews/HotCornersMap.qml
    qml/previews/InsertHintPreview.qml
    qml/previews/MockWindow.qml
    qml/previews/ModKeyPreview.qml
    qml/previews/MonitorArrangement.qml
    qml/previews/MonitorFrame.qml
    qml/previews/MotionOverview.qml
    qml/previews/NewColumnDiagram.qml
    qml/previews/PaintFrame.qml
    qml/previews/PlacementDiagram.qml
    qml/previews/PlasmaPreview.qml
    qml/previews/ProfileGlance.qml
    qml/previews/StripPreview.qml
    qml/previews/StyleBackdrop.qml
    qml/previews/StyleFrame.qml
    qml/previews/StyleTile.qml
    qml/previews/SignalBadge.qml
    qml/previews/TabIndicatorPreview.qml
    qml/previews/MiniBadge.qml
    qml/previews/MiniPill.qml
    qml/previews/MiniTask.qml
    qml/previews/MiniTaskbar.qml
    qml/previews/TaskbarPreview.qml
    qml/previews/TitlebarDragDemo.qml
    qml/previews/WarpDemo.qml
    qml/previews/WorkspaceStack.qml
)
ecm_target_qml_sources(konveyor_settings PATH sections SOURCES
    qml/sections/BlockSliderRow.qml
    qml/sections/BorderColorsRow.qml
    qml/sections/BorderGallerySection.qml
    qml/sections/BorderLayerCard.qml
    qml/sections/BorderLayerChoices.qml
    qml/sections/DriveButton.qml
    qml/sections/FireOnRow.qml
    qml/sections/CornerRule.js
    qml/sections/CornersSection.qml
    qml/sections/EventEffectRow.qml
    qml/sections/EventEffectsSection.qml
    qml/sections/InsertHintSection.qml
    qml/sections/LayoutKeys.js
    qml/sections/LayoutOverrides.qml
    qml/sections/PlacementSection.qml
    qml/sections/RingSection.qml
    qml/sections/ScopedRow.qml
    qml/sections/ScopedSwitch.qml
    qml/sections/SizesSection.qml
    qml/sections/TabIndicatorSection.qml
    qml/sections/TaskbarLookSection.qml
    qml/sections/TaskbarPinsSection.qml
    qml/sections/TaskbarWindowsSection.qml
    qml/sections/TaskbarWorkspacesSection.qml
    qml/sections/TouchDeviceSection.qml
    qml/sections/ValueSlider.qml
)

target_include_directories(konveyor_settings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/preview ${CMAKE_CURRENT_SOURCE_DIR}/store ${CMAKE_CURRENT_SOURCE_DIR}/system)
target_compile_options(konveyor_settings PRIVATE -Wall -Wextra -Werror)
target_link_options(konveyor_settings PRIVATE LINKER:--no-undefined)
target_link_libraries(konveyor_settings PRIVATE
    konveyor_settings_model
    konveyor_config
    konveyor_layout
    konveyor_render
    KF6::ConfigGui
    KF6::ConfigQml
    KF6::CoreAddons
    KF6::Service
    Qt6::Quick
    Qt6::DBus
    Qt6::GuiPrivate
)

ecm_finalize_qml_module(konveyor_settings DESTINATION ${KDE_INSTALL_QMLDIR})
