find_package(KWin REQUIRED COMPONENTS kwineffects)
find_package(KDecoration3 REQUIRED)
find_package(KF6 REQUIRED COMPONENTS CoreAddons GlobalAccel Config ColorScheme Notifications I18n)

kcoreaddons_add_plugin(konveyor_effect INSTALL_NAMESPACE "kwin/effects/plugins")

file(GLOB_RECURSE KONVEYOR_EFFECT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
target_sources(konveyor_effect PRIVATE ${KONVEYOR_EFFECT_SOURCES})
target_include_directories(konveyor_effect PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(konveyor_effect PRIVATE KONVEYOR_VERSION="${PROJECT_VERSION}")
target_compile_options(konveyor_effect PRIVATE -Wall -Wextra -Werror)
target_link_options(konveyor_effect PRIVATE LINKER:--no-undefined)

target_link_libraries(konveyor_effect PRIVATE
    konveyor_core
    konveyor_shortcuts
    KWin::kwin
    KDecoration3::KDecoration
    KF6::CoreAddons
    KF6::GlobalAccel
    KF6::ConfigCore
    KF6::ColorScheme
    KF6::Notifications
    KF6::I18n
    Qt6::DBus
)
