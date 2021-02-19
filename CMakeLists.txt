# CMake 3.9 or newer
cmake_minimum_required(VERSION 3.9)

# Compiling options
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "-ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
if(WIN32)
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_EXE_LINKER_FLAGS "-static")
endif(WIN32)

# Adding fcpp
option(FCPP_BUILD_GL "Build OpenGL features." ON) # this tells CMake to compile fcpp's OpenGL features
add_subdirectory(./fcpp/src/)
target_compile_options(fcpp PUBLIC "-DFCPP_CONFIGURATION=FCPP_CONFIGURATION_DEPENDENT")


# MIOSIX SIMULATION
project(miosix_simulation
    VERSION 0.1.0
    DESCRIPTION "Simulation of the Miosix Case Study."
)
add_executable(miosix_simulation ./src/simulation.cpp)
set_target_properties(miosix_simulation PROPERTIES VERSION &{PROJECT_VERSION})
target_link_libraries(miosix_simulation PRIVATE fcpp)
target_include_directories(miosix_simulation PRIVATE ./)
copyGraphicsFolders(miosix_simulation $<TARGET_FILE_DIR:fcpp> $<TARGET_FILE_DIR:miosix_simulation>)
copyTextureFolder(miosix_simulation ${PROJECT_SOURCE_DIR} $<TARGET_FILE_DIR:miosix_simulation>)
