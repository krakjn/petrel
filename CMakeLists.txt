cmake_minimum_required(VERSION 3.10)
project(petrel VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Fast-DDS libraries
find_library(FASTDDS_LIB fastdds PATHS /usr/lib /usr/local/lib REQUIRED)
find_library(FASTCDR_LIB fastcdr PATHS /usr/lib /usr/local/lib REQUIRED)

# Find Fast-DDS headers
find_path(FASTDDS_INCLUDE_DIR fastdds/dds/domain/DomainParticipantFactory.hpp PATHS /usr/include /usr/local/include REQUIRED)

# Include directories
include_directories(include)
include_directories(${FASTDDS_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gen)

# Petrel is now header-only, no library to build
# Just set up an interface library for installation
add_library(petrel INTERFACE)
target_include_directories(petrel INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# HelloWorld IDL generated sources
set(HELLOWORLD_SOURCES
    gen/HelloWorldPubSubTypes.cxx
    gen/HelloWorldTypeObjectSupport.cxx
)

# IDL-based publisher and subscriber examples
add_executable(publisher examples/publisher.cpp ${HELLOWORLD_SOURCES})
target_include_directories(publisher PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/gen
)
target_link_libraries(publisher
    ${FASTDDS_LIB}
    ${FASTCDR_LIB}
)

add_executable(subscriber examples/subscriber.cpp ${HELLOWORLD_SOURCES})
target_include_directories(subscriber PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/gen
)
target_link_libraries(subscriber
    ${FASTDDS_LIB}
    ${FASTCDR_LIB}
)

# Install rules
install(TARGETS petrel
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/petrel
    DESTINATION include
)

