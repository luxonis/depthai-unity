cmake_minimum_required(VERSION 3.2)

# Add depthai-core dependency
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/depthai-core EXCLUDE_FROM_ALL)

# Create a project with name 'myapp'
set(TARGET_NAME depthai-unity)
project(${TARGET_NAME} VERSION "1.0.0" LANGUAGES CXX C)

# Set default build type depending on context
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git" AND NOT DEFINED ENV{CI})
    set(default_build_type "Debug")
endif()
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Set default installation directory
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory" FORCE)
endif()

# Dependencies (optional, only used for example)
find_package(OpenCV REQUIRED)

find_package(FP16 REQUIRED)
# add_library(utility src/utility.cpp)
# target_link_libraries(utility FP16::fp16 ${OpenCV_LIBS})

# Create unity library
add_library(${TARGET_NAME}
    src/utility.cpp
    src/device/DeviceManager.cpp
    src/device/Streams.cpp
    src/device/PointCloudVFX.cpp
    src/predefined/FaceDetector.cpp
    src/predefined/ObjectDetector.cpp
    src/predefined/BodyPose.cpp
    src/predefined/FaceEmotion.cpp
    src/Depth.cpp
)

# Add include directories
target_include_directories(${TARGET_NAME}
    PUBLIC
        # Build time path to include directories
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
)


target_link_libraries(${TARGET_NAME}
    PUBLIC
        FP16::fp16 
        ${OpenCV_LIBS}
    PRIVATE
        depthai::opencv
)


# Set compiler features (c++14), and disables extensions (g++14)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 14)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_EXTENSIONS OFF)
