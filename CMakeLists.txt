cmake_minimum_required(VERSION 3.24)

project(konveyor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(KONVEYOR_BUILD_EFFECT "Build the KWin effect plugin" ON)
option(KONVEYOR_BUILD_TESTS "Build unit tests" ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui DBus Test)

add_subdirectory(src/core)
add_subdirectory(src/shortcuts)
add_subdirectory(src/cli)
add_subdirectory(src/components)
add_subdirectory(src/settings)

if(KONVEYOR_BUILD_EFFECT)
    add_subdirectory(src/effect)
endif()

if(KONVEYOR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(FILES data/default-config.kdl DESTINATION ${KDE_INSTALL_DATADIR}/konveyor)
install(PROGRAMS src/cheatsheet/konveyor-cheatsheet DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES src/cheatsheet/Cheatsheet.qml DESTINATION ${KDE_INSTALL_DATADIR}/konveyor/cheatsheet)
install(PROGRAMS extras/packaging/konveyor-rebuild DESTINATION ${KDE_INSTALL_LIBDIR}/konveyor)
