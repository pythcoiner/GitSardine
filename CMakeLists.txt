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

# Find libgit2 and its dependencies (required)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)
pkg_check_modules(PCRE2 REQUIRED IMPORTED_TARGET libpcre2-8)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET openssl)
pkg_check_modules(LIBSSH2 IMPORTED_TARGET libssh2)
message(STATUS "Found libgit2 ${LIBGIT2_VERSION}")

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

target_link_libraries(gitsardine PRIVATE
    PkgConfig::LIBGIT2
    PkgConfig::PCRE2
    PkgConfig::OPENSSL
    $<$<BOOL:${LIBSSH2_FOUND}>:PkgConfig::LIBSSH2>
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
