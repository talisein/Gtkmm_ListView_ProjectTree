# Defines the minimal version allowed of cmake
cmake_minimum_required (VERSION 3.20)

# Set the C++ standard to c++20
set (CMAKE_CXX_STANDARD 20
     CACHE STRING "C++ Standard version")

# Set to "ON" to debug makefile commands
set (CMAKE_VERBOSE_MAKEFILE ON
     CACHE BOOL "Show makefiles commands on build")

project (databridgeneo)

set (GUI_TARGET_NAME "test" CACHE
     STRING "Name of the gui executable")

# Adds the executable for the GUI
add_executable(${GUI_TARGET_NAME}
#    main.cpp
#    ProjectTree.cpp
#    ProjectTree.hh
#    MainWindow.cpp
#    MainWindow.hh
     example_listview_treelist.cc)

# Add extra flags to the compiler
target_compile_options(${GUI_TARGET_NAME}
    PRIVATE
        -Werror
        -Wall
        -Wextra)

# Finds PkgConfig and dependecies for the targets
find_package (PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

# Adds several directories to the include path
target_include_directories (${GUI_TARGET_NAME}
    PRIVATE
        ${GTKMM_INCLUDE_DIRS})

# Links the libs
target_link_libraries(${GUI_TARGET_NAME}
    PRIVATE
        ${GTKMM_LIBRARIES})

# Adds flags to the compiler
target_compile_options (${GUI_TARGET_NAME}
    PRIVATE
        ${GTKMM_OTHER})
