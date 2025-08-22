cmake_minimum_required(VERSION 3.16)
project(realtime VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets StateMachine PrintSupport SerialPort)

set(PROJECT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/qcustomplot.cpp
    src/qcustomplot.h
    src/settingswindow.cpp
    src/settingswindow.h
    src/plotwindow.cpp
    src/plotwindow.h
    src/mycustomplot.h
    resources/resources.qrc
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(realtime
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(realtime
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(realtime PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::StateMachine
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::SerialPort
)

# Platform-specific settings
if(APPLE)
    set_target_properties(realtime PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.yourdomain.realtime"
    )

# Windows-specific (keep if you want Windows builds)
elseif(WINDOWS)
    set_target_properties(realtime PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

include(GNUInstallDirs)
install(TARGETS realtime
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_definitions(-w)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(realtime)
endif()
