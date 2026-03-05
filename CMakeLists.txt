cmake_minimum_required(VERSION 3.20)
project(sysmon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -fsanitize=address,undefined")

file(GLOB SOURCES "src/*.cpp")
add_executable(sysmon ${SOURCES})
target_include_directories(sysmon PRIVATE include)
target_compile_options(sysmon PRIVATE -Wall -Wextra)

# Install rules
include(GNUInstallDirs)
install(TARGETS sysmon DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES sysmon.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(DIRECTORY themes/ DESTINATION ${CMAKE_INSTALL_DATADIR}/sysmon/themes
        FILES_MATCHING PATTERN "*.theme")
