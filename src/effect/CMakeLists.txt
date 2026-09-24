find_package(KWin 6.7 REQUIRED COMPONENTS kwineffects)
find_package(KDecoration3 REQUIRED)
find_package(KF6 REQUIRED COMPONENTS CoreAddons GlobalAccel Config ColorScheme Notifications I18n)
find_package(XCB REQUIRED COMPONENTS XCB ICCCM)

kcoreaddons_add_plugin(konveyor_effect INSTALL_NAMESPACE "kwin/effects/plugins")

file(GLOB_RECURSE KONVEYOR_EFFECT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
target_sources(konveyor_effect PRIVATE ${KONVEYOR_EFFECT_SOURCES})
target_include_directories(konveyor_effect PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(konveyor_effect PRIVATE KONVEYOR_VERSION="${PROJECT_VERSION}" KONVEYOR_BINDIR="${KDE_INSTALL_FULL_BINDIR}")
get_target_property(KONVEYOR_KWIN_INCLUDES KWin::kwin INTERFACE_INCLUDE_DIRECTORIES)
find_file(KONVEYOR_KWIN_WINDOW_HEADER window.h PATHS ${KONVEYOR_KWIN_INCLUDES} NO_DEFAULT_PATH NO_CACHE REQUIRED)
file(READ ${KONVEYOR_KWIN_WINDOW_HEADER} KONVEYOR_KWIN_WINDOW)
string(FIND "${KONVEYOR_KWIN_WINDOW}" "takesAsyncFocus" KONVEYOR_KWIN_ASYNC_FOCUS_AT)
if(NOT KONVEYOR_KWIN_ASYNC_FOCUS_AT EQUAL -1)
    target_compile_definitions(konveyor_effect PRIVATE KONVEYOR_KWIN_ASYNC_FOCUS)
endif()
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
    XCB::XCB
    XCB::ICCCM
)
