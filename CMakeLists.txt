project(simplecm)

cmake_minimum_required(VERSION 2.8)

set(SIMPLECM_VERSION_MAJOR 0)
set(SIMPLECM_VERSION_MINOR 1)
set(SIMPLECM_VERSION_PATCH 0)

set(SIMPLECM_VERSION "${SIMPLECM_VERSION_MAJOR}.${SIMPLECM_VERSION_MINOR}.${SIMPLECM_VERSION_PATCH}")

if (USE_QT4)
    set(QT_VERSION_MAJOR "4")
    find_package(TelepathyQt4 0.9.5 REQUIRED)
    find_package(TelepathyQt4Service 0.9.5 REQUIRED)

    find_package(Qt4 4.6.0 REQUIRED QtCore QtDBus)
else()
    set(QT_VERSION_MAJOR "5")
    find_package(TelepathyQt5 0.9.5 REQUIRED)
    find_package(TelepathyQt5Service 0.9.5 REQUIRED)

    find_package(Qt5 REQUIRED COMPONENTS Core DBus Network Xml WebKit WebKitWidgets)
    
endif()

include(GNUInstallDirs)

add_definitions(-DSIMPLECM_LIBRARY)

set(simplecm_HEADERS
    connection.h
    protocol.h
    textchannel.h
    simplecm_export.h
)

set(simplecm_SOURCES
    main.cpp
    connection.cpp
    protocol.cpp
    textchannel.cpp
)

add_executable(simplecm-qt ${simplecm_SOURCES})

if (USE_QT4)
    target_include_directories(simplecm-qt PRIVATE
        ${QT_INCLUDES}
        ${TELEPATHY_QT4_INCLUDE_DIR}
    )
    target_link_libraries(simplecm-qt
        ${QT_QTCORE_LIBRARY}
        ${QT_QTDBUS_LIBRARY}
        ${TELEPATHY_QT4_LIBRARIES}
        ${TELEPATHY_QT4_SERVICE_LIBRARIES}
	
    )
else()
    target_include_directories(simplecm-qt PRIVATE
        ${TELEPATHY_QT5_INCLUDE_DIR}
    )

    target_link_libraries(simplecm-qt
        Qt5::Core
        Qt5::DBus
        Qt5::WebKit
        Qt5::WebKitWidgets
        ${TELEPATHY_QT5_LIBRARIES}
        ${TELEPATHY_QT5_SERVICE_LIBRARIES}
    )
endif()

set_target_properties(simplecm-qt PROPERTIES AUTOMOC TRUE)

# install(
#     TARGETS simplecm-qt
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     COMPONENT Library
# )
#
# install(
#     FILES ${simplecm_HEADERS}
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/simplecm
#     COMPONENT Devel
# )
#
# install(
#     DIRECTORY SimpleCM
#     DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
#     COMPONENT Devel
# )
