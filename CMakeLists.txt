cmake_minimum_required(VERSION 3.21)
project(tvdeck VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 6.4 is the floor so this builds on common distros, but Netflix tracks
# recent Chromium releases closely: ship with Qt 6.8+ (Chromium 122+).
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick DBus WebEngineQuick)
find_package(SDL2 REQUIRED CONFIG)

qt_standard_project_setup()
add_compile_definitions(PROJECT_VERSION_STRING="${PROJECT_VERSION}")

qt_add_executable(tvdeck
    src/main.cpp
    src/input/GamepadInput.cpp      src/input/GamepadInput.h
    src/input/ActionRouter.cpp      src/input/ActionRouter.h
    src/input/VirtualPointer.cpp    src/input/VirtualPointer.h
    src/input/JogDial.h
    src/services/ServiceRegistry.cpp src/services/ServiceRegistry.h
    src/platform/Platform.cpp       src/platform/Platform.h
)

qt_add_qml_module(tvdeck
    URI TVDeck
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/Main.qml
        qml/Launcher.qml
        qml/ServiceView.qml
        qml/components/ButtonHints.qml
        qml/components/ButtonGlyph.qml
        qml/components/ConfirmDialog.qml
        qml/components/Osd.qml
)

# Service definitions, bindings and injected page scripts.
qt_add_resources(tvdeck "data"
    PREFIX /
    FILES
        services/netflix.json
        config/bindings.json
        scripts/common/bridge.js
        scripts/common/spatial-nav.js
        scripts/netflix/netflix.js
)

target_link_libraries(tvdeck PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::DBus Qt6::WebEngineQuick
    SDL2::SDL2
)

include(GNUInstallDirs)
install(TARGETS tvdeck RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/io.github.sparatan117.TVDeck.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(PROGRAMS scripts/install-widevine.sh DESTINATION ${CMAKE_INSTALL_BINDIR} RENAME tvdeck-install-widevine)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_jogdial tests/tst_jogdial.cpp)
    target_include_directories(tst_jogdial PRIVATE src)
    target_link_libraries(tst_jogdial PRIVATE Qt6::Test)
    add_test(NAME jogdial COMMAND tst_jogdial)
endif()
