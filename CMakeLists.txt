cmake_minimum_required(VERSION 3.10)
project(petrel VERSION 0.1.0 LANGUAGES CXX)

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

install(FILES include/petrel.h
    DESTINATION include
)

# Install Fast-DDS libraries and headers (bundled)
if(UNIX AND NOT APPLE)
    # Linux: Install to /usr
    # Get the directory containing the library files
    get_filename_component(FASTDDS_LIB_DIR ${FASTDDS_LIB} DIRECTORY)
    get_filename_component(FASTCDR_LIB_DIR ${FASTCDR_LIB} DIRECTORY)
    
    # Install all Fast-DDS library files (including symlinks)
    file(GLOB FASTDDS_LIBS "${FASTDDS_LIB_DIR}/libfastdds.so*")
    file(GLOB FASTCDR_LIBS "${FASTCDR_LIB_DIR}/libfastcdr.so*")
    
    install(FILES ${FASTDDS_LIBS} ${FASTCDR_LIBS}
        DESTINATION lib
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    
    # Install headers
    install(DIRECTORY ${FASTDDS_INCLUDE_DIR}/fastdds
        DESTINATION include
    )
    install(DIRECTORY ${FASTDDS_INCLUDE_DIR}/fastcdr
        DESTINATION include
    )
    
    # Install fastddsgen
    find_program(FASTDDSGEN fastddsgen)
    if(FASTDDSGEN)
        install(PROGRAMS ${FASTDDSGEN}
            DESTINATION bin
        )
    endif()
    
elseif(APPLE)
    # macOS: Install to /usr/local
    get_filename_component(FASTDDS_LIB_DIR ${FASTDDS_LIB} DIRECTORY)
    get_filename_component(FASTCDR_LIB_DIR ${FASTCDR_LIB} DIRECTORY)
    
    # Install all Fast-DDS library files (including symlinks)
    file(GLOB FASTDDS_LIBS "${FASTDDS_LIB_DIR}/libfastdds*.dylib")
    file(GLOB FASTCDR_LIBS "${FASTCDR_LIB_DIR}/libfastcdr*.dylib")
    
    install(FILES ${FASTDDS_LIBS} ${FASTCDR_LIBS}
        DESTINATION local/lib
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
    
    # Install headers
    install(DIRECTORY ${FASTDDS_INCLUDE_DIR}/fastdds
        DESTINATION local/include
    )
    install(DIRECTORY ${FASTDDS_INCLUDE_DIR}/fastcdr
        DESTINATION local/include
    )
    
    # Install fastddsgen
    find_program(FASTDDSGEN fastddsgen)
    if(FASTDDSGEN)
        install(PROGRAMS ${FASTDDSGEN}
            DESTINATION local/bin
        )
    endif()
    
elseif(WIN32)
    # Windows: Install to Program Files
    get_filename_component(FASTDDS_LIB_DIR ${FASTDDS_LIB} DIRECTORY)
    get_filename_component(FASTCDR_LIB_DIR ${FASTCDR_LIB} DIRECTORY)
    
    # Install DLL files
    file(GLOB FASTDDS_DLLS "${FASTDDS_LIB_DIR}/../bin/fastdds*.dll")
    file(GLOB FASTCDR_DLLS "${FASTCDR_LIB_DIR}/../bin/fastcdr*.dll")
    
    install(FILES ${FASTDDS_DLLS} ${FASTCDR_DLLS}
        DESTINATION bin
    )
    
    # Install lib files
    file(GLOB FASTDDS_LIBS "${FASTDDS_LIB_DIR}/fastdds*.lib")
    file(GLOB FASTCDR_LIBS "${FASTCDR_LIB_DIR}/fastcdr*.lib")
    
    install(FILES ${FASTDDS_LIBS} ${FASTCDR_LIBS}
        DESTINATION lib
    )
    
    # Install headers
    install(DIRECTORY ${FASTDDS_INCLUDE_DIR}/fastdds
        DESTINATION include
    )
    install(DIRECTORY ${FASTDDS_INCLUDE_DIR}/fastcdr
        DESTINATION include
    )
    
    # Install fastddsgen
    find_program(FASTDDSGEN fastddsgen.bat)
    if(FASTDDSGEN)
        install(PROGRAMS ${FASTDDSGEN}
            DESTINATION bin
        )
    endif()
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "petrel")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Petrel")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast-DDS Simplified - A clean, type-safe C++ wrapper around eProsima Fast-DDS")
set(CPACK_PACKAGE_CONTACT "maintainer@petrel.dev")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Platform-specific packaging
if(UNIX AND NOT APPLE)
    # Linux - DEB and TGZ
    set(CPACK_GENERATOR "DEB;TGZ")
    
    # DEB specific
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Petrel Maintainer")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "default-jre | java11-runtime")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    endif()
    
    # Set package file names
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-linux-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    
elseif(APPLE)
    # macOS - TGZ
    set(CPACK_GENERATOR "TGZ")
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-darwin-arm64")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-darwin-x86_64")
    endif()
    
elseif(WIN32)
    # Windows - ZIP
    set(CPACK_GENERATOR "ZIP")
    
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64")
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-arm64")
    else()
        set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-x86_64")
    endif()
endif()

include(CPack)

