set(PROJECT_NAME aclTest)

project(${PROJECT_NAME})
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

find_program(MAKE_EXE NAMES gmake nmake make)

#message(STATUS "CMAKE_CURRENT_BINARY_DIR:"${CMAKE_CURRENT_BINARY_DIR})

## Additional CMake modules
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

## Add required library
#find_package(Gmock REQUIRED)
#message(STATUS "GMOCK_INCLUDE_DIR: " ${GMOCK_INCLUDE_DIR})
#message(STATUS "GMOCK_LIBRARIES: " ${GMOCK_LIBRARIES})

find_package(Gtest REQUIRED)
message(STATUS "GTEST_INCLUDE_DIR: " ${GTEST_INCLUDE_DIR})
message(STATUS "GTEST_LIBRARIES: " ${GTEST_LIBRARIES})
message(STATUS "======================================")

## Add definitions
add_definitions(-std=c++11)
add_definitions(-g -O0)

set(SWSS_PATH "${CMAKE_CURRENT_BINARY_DIR}/sonic-swss/")
set(SWSSCOMM_PATH "${CMAKE_CURRENT_BINARY_DIR}/external/src/project_swsscommon/")
set(SAIREDIS_PATH "${CMAKE_CURRENT_BINARY_DIR}/sonic-sairedis")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${SWSS_PATH}/orchagent/
                    ${SWSSCOMM_PATH}/common
                    ${SWSSCOMM_PATH}
                    ${SAIREDIS_PATH}
                    ${SAIREDIS_PATH}/meta
                    ${SAIREDIS_PATH}/SAI/inc
                    ${SAIREDIS_PATH}/SAI/meta
                    ${SAIREDIS_PATH}/SAI/experimental
)

## libswsscommon ##################################################
ExternalProject_Add(project_swsscommon
  GIT_REPOSITORY https://github.com/Azure/sonic-swss-common.git
  GIT_TAG master
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install/
  CONFIGURE_COMMAND sh -c "./autogen.sh && ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/install/swsscommon"
  UPDATE_COMMAND ""
  BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_swsscommon install_dir)
message(STATUS "swsscommon install_dir: " ${install_dir})
LINK_DIRECTORIES(${install_dir}/swsscommon/lib)
# copy swsscommon header to ${install_dir}/swss
file(GLOB SWSSCOMM_HEADER
    ${CMAKE_CURRENT_BINARY_DIR}/external/src/project_swsscommon/common/*.h
    ${CMAKE_CURRENT_BINARY_DIR}/external/src/project_swsscommon/common/json.hpp
)
message(STATUS "SWSSCOMM_HEADER:" ${SWSSCOMM_HEADER})
file(INSTALL ${SWSSCOMM_HEADER}
     DESTINATION ${install_dir}/include/swss
)
set(SWSSCOMMON_INSTALL_PATH ${install_dir})

## sairedis ####################################################
# for libsaimeta / libsaimetadata
LINK_DIRECTORIES(${SWSSCOMMON_INSTALL_PATH}/sairedis/lib)
################################################################

## liborchagent #################################################
file(GLOB ORCH_SRCS
    ${SWSS_PATH}/orchagent/orchdaemon.cpp
    ${SWSS_PATH}/orchagent/orch.cpp
    ${SWSS_PATH}/orchagent/notifications.cpp 
    ${SWSS_PATH}/orchagent/routeorch.cpp
    ${SWSS_PATH}/orchagent/neighorch.cpp
    ${SWSS_PATH}/orchagent/intfsorch.cpp
    ${SWSS_PATH}/orchagent/portsorch.cpp
    ${SWSS_PATH}/orchagent/copporch.cpp
    ${SWSS_PATH}/orchagent/tunneldecaporch.cpp
    ${SWSS_PATH}/orchagent/qosorch.cpp
    ${SWSS_PATH}/orchagent/bufferorch.cpp
    ${SWSS_PATH}/orchagent/mirrororch.cpp
    ${SWSS_PATH}/orchagent/fdborch.cpp
    ${SWSS_PATH}/orchagent/aclorch.cpp
    ${SWSS_PATH}/orchagent/saihelper.cpp
    ${SWSS_PATH}/orchagent/switchorch.cpp
#    ${SWSS_PATH}/orchagent/pfcwdorch.cpp
#    ${SWSS_PATH}/orchagent/pfcactionhandler.cpp
    ${SWSS_PATH}/orchagent/crmorch.cpp
    ${SWSS_PATH}/orchagent/request_parser.cpp
    ${SWSS_PATH}/orchagent/vrforch.cpp
    ${SWSS_PATH}/orchagent/countercheckorch.cpp
    ${SWSS_PATH}/orchagent/vxlanorch.cpp
    ${SWSS_PATH}/orchagent/vnetorch.cpp
    ${SWSS_PATH}/orchagent/dtelorch.cpp
    ${SWSS_PATH}/orchagent/flexcounterorch.cpp
    ${SWSS_PATH}/orchagent/watermarkorch.cpp
)

file(GLOB ORCH_HEADERS
    ${SWSS_PATH}/orchagent/acltable.h
    ${SWSS_PATH}/orchagent/aclorch.h
    ${SWSS_PATH}/orchagent/bufferorch.h
    ${SWSS_PATH}/orchagent/copporch.h
    ${SWSS_PATH}/orchagent/directory.h
    ${SWSS_PATH}/orchagent/fdborch.h
    ${SWSS_PATH}/orchagent/intfsorch.h
    ${SWSS_PATH}/orchagent/mirrororch.h
    ${SWSS_PATH}/orchagent/neighorch.h
    ${SWSS_PATH}/orchagent/notifications.h
    ${SWSS_PATH}/orchagent/observer.h
    ${SWSS_PATH}/orchagent/orch.h
    ${SWSS_PATH}/orchagent/orchdaemon.h
    ${SWSS_PATH}/orchagent/pfcactionhandler.h
    ${SWSS_PATH}/orchagent/pfcwdorch.h
    ${SWSS_PATH}/orchagent/port.h
    ${SWSS_PATH}/orchagent/portsorch.h
    ${SWSS_PATH}/orchagent/qosorch.h
    ${SWSS_PATH}/orchagent/routeorch.h
    ${SWSS_PATH}/orchagent/saihelper.h
    ${SWSS_PATH}/orchagent/switchorch.h
    ${SWSS_PATH}/orchagent/swssnet.h
    ${SWSS_PATH}/orchagent/tunneldecaporch.h
    ${SWSS_PATH}/orchagent/crmorch.h
    ${SWSS_PATH}/orchagent/request_parser.h
    ${SWSS_PATH}/orchagent/vrforch.h
    ${SWSS_PATH}/orchagent/dtelorch.h
    ${SWSS_PATH}/orchagent/countercheckorch.h
    ${SWSS_PATH}/orchagent/vxlanorch.h
    ${SWSS_PATH}/orchagent/vnetorch.h
    ${SWSS_PATH}/orchagent/flexcounterorch.h
    ${SWSS_PATH}/orchagent/watermarkorch.h
)

add_library(orchagent
    ${ORCH_SRCS}
    ${ORCH_HEADERS}
)

target_include_directories(orchagent PUBLIC
    ${SWSS_PATH}/orchagent/
    ${SAIREDIS_PATH}/SAI/inc/
    ${SAIREDIS_PATH}/lib/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/mock/sonic
    ${SWSSCOMMON_INSTALL_PATH}/include
)

target_link_libraries(orchagent
    hiredis swsscommon sairedis saimetadata)

#################################################################

## aclTest ######################################################
file(GLOB TESTS
    main.cpp
#    mock_sai.cpp
#    mock_orch.cpp
    test_aclorch.cpp
)

file(GLOB SRCS
#    ${SWSS_PATH}/orchagent/aclorch.cpp
)

file(GLOB INCLS
    ${SWSS_PATH}/orchagent/aclorch.h
    ${SWSS_PATH}/orchagent/acltable.h
)

source_group(src\\include FILES ${INCLS})
source_group(src FILES ${SRCS})
source_group(test FILES ${TESTS})

#set_source_files_properties(${SRCS}
#   PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${PROJECT_NAME}
   ${SRCS}
   ${INCLS}
   ${TESTS}
)

add_dependencies(${PROJECT_NAME} project_swsscommon)

#target_include_directories(${PROJECT_NAME}  PRIVATE ${GMOCK_INCLUDE_DIR})
#target_link_libraries(${PROJECT_NAME}  ${GMOCK_LIBRARIES})

#target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/install/swsscommon/lib/libswsscommon.a)

LINK_DIRECTORIES(${SWSSCOMMON_INSTALL_PATH}/sairedis/lib)
#target_link_directories(${PROJECT_NAME} PRIVATE ${SWSSCOMMON_INSTALL_PATH}/sairedis/lib)

target_link_libraries(${PROJECT_NAME} ${GTEST_LIBRARIES})
target_link_libraries(${PROJECT_NAME} pthread hiredis swsscommon sairedis saimetadata orchagent)
