cmake_minimum_required(VERSION 3.25.1)


add_subdirectory(../glm build/glm)

SET(GLFW_INSTALL ON)

# SET(CMAKE_BUILD_TYPE RELEASE)

SET(CMAKE_BUILD_TYPE DEBUG)

project(scene)

find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})

file(GLOB Sources *.cpp)
file(GLOB Headers *.hpp)

if(CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/debug")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/release")
endif()

add_executable(${PROJECT_NAME} ${Sources} ${Headers})

target_link_libraries(${PROJECT_NAME} ${GLEW_LIBRARIES})

find_package(GLEW REQUIRED)
target_link_libraries(${PROJECT_NAME} GL)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} glm)
