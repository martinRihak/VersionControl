# Minimální požadovaná verze CMake
cmake_minimum_required(VERSION 3.8)

# Název projektu
project(VersionControl)

# Nastavení standardu C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Kompilační příznaky
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
option(DISABLE_ASAN "Do not use Address sanitizer" OFF)

# Povolení assercí v release módu
add_compile_options(-UNDEBUG)

# Pokud není zakázán Address Sanitizer
if(NOT DISABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# Dodatečné kompilátorské příznaky
add_compile_options(-Wall -Wextra -pedantic)

# Hledání OpenSSL
find_package(OpenSSL REQUIRED)

# Hledání zlib
find_package(ZLIB REQUIRED)

add_executable(VersionControl
    src/main.cpp
    src/vcs/GitInitializer.cpp
    src/vcs/VersionControl.cpp
)

target_include_directories(VersionControl PRIVATE include/vcs)

target_link_libraries(VersionControl PRIVATE OpenSSL::SSL OpenSSL::Crypto ZLIB::ZLIB)

set_target_properties(VersionControl PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../app
)