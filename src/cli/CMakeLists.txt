file(GLOB KONVEYOR_CLI_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(konveyor ${KONVEYOR_CLI_SOURCES})
target_link_libraries(konveyor PRIVATE konveyor_ipc konveyor_config konveyor_shortcuts Qt6::DBus)
target_compile_options(konveyor PRIVATE -Wall -Wextra -Wpedantic -Werror)
install(TARGETS konveyor DESTINATION ${KDE_INSTALL_BINDIR})
