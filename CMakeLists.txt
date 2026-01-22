cmake_minimum_required(VERSION 3.17)
project(gitsardine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt6 - use system Qt or custom path
if(DEFINED ENV{QT_DIR})
    set(CMAKE_PREFIX_PATH $ENV{QT_DIR})
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Try to find libgit2 (optional)
find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBGIT2 libgit2)
endif()

if(LIBGIT2_FOUND)
    message(STATUS "Found libgit2 - using native git library")
    add_compile_definitions(USE_LIBGIT2)
else()
    message(STATUS "libgit2 not found - using QProcess for git commands")
endif()

# Add qontrol
add_subdirectory(deps/qontrol)

# Sources
set(SOURCES
    src/main.cpp
    src/core/Result.h
    src/config/Config.cpp
    src/git/GitRepository.cpp
    src/git/GitStatus.h
    src/models/RepoModel.cpp
    src/models/FolderTreeModel.cpp
    src/workers/GitWorker.cpp
    src/widgets/RepoTreeWidget.cpp
    src/widgets/ChangesTreeWidget.cpp
    src/widgets/BranchSelector.cpp
    src/widgets/StatusBar.cpp
    src/widgets/DiffViewerDialog.cpp
    src/widgets/SetupDialog.cpp
    src/screens/MainScreen.cpp
    src/controller/AppController.cpp
)

add_executable(gitsardine ${SOURCES})

target_include_directories(gitsardine PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/resources
)

if(LIBGIT2_FOUND)
    target_include_directories(gitsardine PRIVATE ${LIBGIT2_INCLUDE_DIRS})
    target_link_libraries(gitsardine PRIVATE ${LIBGIT2_LIBRARIES})
endif()

target_link_libraries(gitsardine PRIVATE
    qontrol
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Install targets
install(TARGETS gitsardine RUNTIME DESTINATION bin)

# Tests
enable_testing()

# Test sources (shared with main app)
set(TEST_COMMON_SOURCES
    src/models/FolderTreeModel.cpp
    src/git/GitStatus.h
)

add_executable(test_tree tests/test_tree.cpp ${TEST_COMMON_SOURCES})
target_include_directories(test_tree PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/resources
)
target_link_libraries(test_tree PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
set_target_properties(test_tree PROPERTIES AUTOMOC ON)
add_test(NAME TreeStructureTest COMMAND test_tree)
