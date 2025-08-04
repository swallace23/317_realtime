cmake_minimum_required(VERSION 3.16)

project(realtime VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS StateMachine)
find_package(Qt6 REQUIRED COMPONENTS PrintSupport)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS SerialPort)




set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        qcustomplot.cpp
        qcustomplot.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(realtime
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        settingswindow.h settingswindow.cpp
        plotwindow.h plotwindow.cpp
        mycustomplot.h
        resources.qrc
    )
  if(APPLE)
    set_target_properties(tst_manual_ios_assets PROPERTIES MACOSX_BUNDLE TRUE)
  endif()
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET realtime APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(realtime SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(realtime
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(realtime PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(realtime PRIVATE Qt6::StateMachine)
target_link_libraries(realtime PRIVATE Qt6::PrintSupport)
target_link_libraries(realtime PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.realtime)
endif()
set_target_properties(realtime PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS realtime
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(realtime)
endif()
