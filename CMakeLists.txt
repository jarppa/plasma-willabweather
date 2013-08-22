# Project Needs a name ofcourse
project(plasma-willabweather)

#set(CMAKE_BACKWARDS_COMPATIBILITY 2.2)

cmake_minimum_required(VERSION 2.8)
 
# Find the required Libaries
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
#find_package(Plasma REQUIRED)
 
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories(
   ${CMAKE_SOURCE_DIR}
   ${CMAKE_BINARY_DIR}
   ${KDE4_INCLUDES}
   ${QT_QTTEST_INCLUDE_DIR}
   )
 
# We add our source code here
set(weather_SRCS 
	plasma-willabweather.cpp
        config.cpp)

# Now make sure all files get to the right place
kde4_add_ui_files(weather_SRCS config.ui)
kde4_add_plugin(plasma_applet_willabweather ${weather_SRCS})
target_link_libraries(plasma_applet_willabweather 
${QT_QTNETWORK_LIBRARY} ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})

install(TARGETS plasma_applet_willabweather
        DESTINATION ${PLUGIN_INSTALL_DIR})
 
install(FILES plasma-applet-willabweather.desktop
        DESTINATION ${SERVICES_INSTALL_DIR})
