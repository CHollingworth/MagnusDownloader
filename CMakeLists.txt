cmake_minimum_required(VERSION 3.27)
project(MagnusDownloader)

set(CMAKE_CXX_STANDARD 17)

add_executable(MagnusDownloader main.cpp)

add_subdirectory(third-party/pugixml)
target_link_libraries(${PROJECT_NAME} pugixml::pugixml)

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${CURL_LIBRARIES})
