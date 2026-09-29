find_package(Qt6 REQUIRED COMPONENTS Qml Quick Widgets)
find_package(KF6 REQUIRED COMPONENTS I18n WindowSystem Config GlobalAccel)
find_package(Plasma REQUIRED)

add_library(konveyor_kontrolpanel STATIC kontrolpanelservice.cpp)
target_include_directories(konveyor_kontrolpanel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(konveyor_kontrolpanel PUBLIC Qt6::Core Qt6::DBus Qt6::Gui KF6::WindowSystem Plasma::Plasma)
target_compile_options(konveyor_kontrolpanel PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(konveyor-kontrol-panel main.cpp)
target_link_libraries(konveyor-kontrol-panel PRIVATE konveyor_kontrolpanel Qt6::Qml Qt6::Quick Qt6::Widgets KF6::I18n KF6::I18nQml KF6::ConfigGui KF6::ConfigQml KF6::GlobalAccel)
target_compile_options(konveyor-kontrol-panel PRIVATE -Wall -Wextra -Wpedantic -Werror)
install(TARGETS konveyor-kontrol-panel DESTINATION ${KDE_INSTALL_BINDIR})
