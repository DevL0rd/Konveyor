find_package(KF6 REQUIRED COMPONENTS KCMUtils CoreAddons)
find_package(Qt6 REQUIRED COMPONENTS Quick)

kcmutils_add_qml_kcm(kcm_konveyor)

target_sources(kcm_konveyor PRIVATE module/konveyorkcm.cpp module/konveyorkcm.h)
target_include_directories(kcm_konveyor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(kcm_konveyor PRIVATE -Wall -Wextra -Werror)
target_link_options(kcm_konveyor PRIVATE LINKER:--no-undefined)

target_link_libraries(kcm_konveyor PRIVATE
    KF6::KCMUtilsQuick
    KF6::CoreAddons
    Qt6::Quick
)

install(FILES $<TARGET_FILE:kcm_konveyor> DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/configs)
