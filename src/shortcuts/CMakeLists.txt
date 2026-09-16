find_package(KF6 REQUIRED COMPONENTS GlobalAccel Config)

set_source_files_properties(${KGLOBALACCEL_DBUS_INTERFACES_DIR}/kf6_org.kde.KGlobalAccel.xml
    PROPERTIES INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/kglobalacceltypes.h CLASSNAME OrgKdeKGlobalAccelInterface)
qt_add_dbus_interface(KONVEYOR_SHORTCUTS_DBUS ${KGLOBALACCEL_DBUS_INTERFACES_DIR}/kf6_org.kde.KGlobalAccel.xml kglobalaccelinterface)

add_library(konveyor_shortcuts STATIC shortcutconflicts.cpp ${KONVEYOR_SHORTCUTS_DBUS})
target_include_directories(konveyor_shortcuts PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(konveyor_shortcuts PUBLIC Qt6::Core Qt6::DBus Qt6::Gui KF6::GlobalAccel KF6::ConfigCore)
target_compile_options(konveyor_shortcuts PRIVATE -Wall -Wextra -Wpedantic -Werror)
set_target_properties(konveyor_shortcuts PROPERTIES POSITION_INDEPENDENT_CODE ON)
