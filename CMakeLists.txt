set(PROJECT_NAME aclTest)

project(${PROJECT_NAME})
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

find_program(MAKE_EXE NAMES gmake nmake make)

#message(STATUS "CMAKE_CURRENT_BINARY_DIR:"${CMAKE_CURRENT_BINARY_DIR})

## Additional CMake modules
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(STATUS "CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})

ExternalProject_Add(project_swsscommon
  GIT_REPOSITORY https://github.com/Azure/sonic-swss-common.git
  GIT_TAG master
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/
  INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install/
  CONFIGURE_COMMAND sh -c "./autogen.sh && ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/install/swsscommon"
  UPDATE_COMMAND rm -rf ./m4 && git pull
  BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(project_swsscommon install_dir)
message(STATUS "swsscommon install_dir: " ${install_dir})
LINK_DIRECTORIES(${install_dir}/swsscommon/lib)

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
add_definitions(-DGTEST_HAS_PTHREADD=0)
add_definitions(-g -O0)

set(SWSS_PATH "${CMAKE_CURRENT_BINARY_DIR}/sonic-swss/")
set(SWSSCOMM_PATH "${CMAKE_CURRENT_BINARY_DIR}/external/src/project_swsscommon/")
set(SAIREDIS_PATH "${CMAKE_CURRENT_BINARY_DIR}/sonic-sairedis")

include_directories(./
                    ${SWSS_PATH}/orchagent/
                    ${SWSSCOMM_PATH}/common/
                    ${SWSSCOMM_PATH}
                    ${SAIREDIS_PATH}
                    ${SAIREDIS_PATH}/meta/
                    ${SAIREDIS_PATH}/SAI/inc/
                    ${SAIREDIS_PATH}/SAI/meta/
)

file(GLOB TESTS
    main.cpp
    mock_sai.cpp
    mock_orch.cpp
    test_aclorch.cpp
)

file(GLOB SRCS
    ${SWSS_PATH}/orchagent/aclorch.cpp
)
#Remove this
#    ${SWSS_PATH}/orchagent/portsorch.cpp
#    ${SWSS_PATH}/orchagent/crmorch.cpp
#    ${SWSS_PATH}/orchagent/neighorch.cpp
#    ${SWSS_PATH}/orchagent/routeorch.cpp
#    ${SWSS_PATH}/orchagent/mirrororch.cpp
#    ${SWSS_PATH}/orchagent/dtelorch.cpp
#    ${SWSS_PATH}/orchagent/bufferorch.cpp
#    ${SWSS_PATH}/orchagent/intfsorch.cpp
#    ${SWSS_PATH}/orchagent/countercheckorch.cpp
#    ${SWSS_PATH}/orchagent/fdborch.cpp
#    ${SWSS_PATH}/orchagent/vnetorch.cpp
#    ${SWSS_PATH}/orchagent/vxlanorch.cpp
#    ${SWSS_PATH}/orchagent/orch.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/logger.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/tokenize.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/ipprefix.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/ipaddress.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/ipaddresses.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/selectabletimer.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/dbconnector.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/table.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/rediscommand.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/redisreply.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/producertable.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/notificationconsumer.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/macaddress.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/redisclient.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/subscriberstatetable.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/consumertablebase.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/consumerstatetable.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/producerstatetable.cpp
#    ${SWSS_PATH}/../sonic-swss-common/common/select.cpp
#    ${SWSS_PATH}/../sonic-sairedis/meta/saiserialize.cpp

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

target_link_libraries(${PROJECT_NAME}  ${GTEST_LIBRARIES})
target_link_libraries(${PROJECT_NAME} pthread hiredis swsscommon)
