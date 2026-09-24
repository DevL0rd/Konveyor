find_package(Qt6 REQUIRED COMPONENTS Qml)

add_executable(konveyor-cheatsheet-viewer viewer.cpp)
target_link_libraries(konveyor-cheatsheet-viewer PRIVATE Qt6::Gui Qt6::Qml)
target_compile_options(konveyor-cheatsheet-viewer PRIVATE -Wall -Wextra -Wpedantic -Werror)
install(TARGETS konveyor-cheatsheet-viewer DESTINATION ${KDE_INSTALL_LIBEXECDIR})

configure_file(konveyor-cheatsheet.in ${CMAKE_CURRENT_BINARY_DIR}/konveyor-cheatsheet @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/konveyor-cheatsheet DESTINATION ${KDE_INSTALL_BINDIR})
install(FILES Cheatsheet.qml DESTINATION ${KDE_INSTALL_DATADIR}/konveyor/cheatsheet)
