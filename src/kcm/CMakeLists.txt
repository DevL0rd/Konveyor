find_package(KF6 REQUIRED COMPONENTS KCMUtils CoreAddons Service)
find_package(Qt6 REQUIRED COMPONENTS Quick GuiPrivate DBus)

kcmutils_add_qml_kcm(kcm_konveyor)

file(GLOB_RECURSE KONVEYOR_KCM_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
target_sources(kcm_konveyor PRIVATE ${KONVEYOR_KCM_SOURCES})
target_include_directories(kcm_konveyor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kcm_konveyor PRIVATE -Wall -Wextra -Werror)
target_link_options(kcm_konveyor PRIVATE LINKER:--no-undefined)

target_link_libraries(kcm_konveyor PRIVATE
    konveyor_config
    konveyor_layout
    KF6::KCMUtilsQuick
    KF6::CoreAddons
    KF6::Service
    Qt6::Quick
    Qt6::DBus
    Qt6::GuiPrivate
)

install(FILES $<TARGET_FILE:kcm_konveyor> DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/configs)
