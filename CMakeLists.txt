set(PROJECT_NAME swss-tests)

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

## Add coverage test flag
include(CodeCoverage)
APPEND_COVERAGE_COMPILER_FLAGS()
include(CTest)

## Add definitions
add_definitions(-std=c++11)
add_definitions(-g -O0)

#set(SWSS_PATH "${CMAKE_CURRENT_BINARY_DIR}/sonic-swss/")
#set(SWSSCOMM_PATH "${CMAKE_CURRENT_BINARY_DIR}/external/src/project_swsscommon/")
#set(SAIREDIS_PATH "${CMAKE_CURRENT_BINARY_DIR}/sonic-sairedis")
set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(SWSS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sonic-swss/")
set(SWSSCOMM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sonic-swss-common/")
set(SAIREDIS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/sonic-sairedis/")
message(STATUS "build path: " ${BUILD_PATH})

include_directories(${SRC_PATH}
                    ${SWSS_PATH}/orchagent
                    ${BUILD_PATH}/install/include
                    ${BUILD_PATH}/install/include/swss
                    #${SWSSCOMM_PATH}/common
                    ${SAIREDIS_PATH}
                    ${SAIREDIS_PATH}/meta
                    ${SAIREDIS_PATH}/SAI/inc
                    ${SAIREDIS_PATH}/SAI/meta
                    ${SAIREDIS_PATH}/SAI/experimental
                    /usr/include/hiredis
)

LINK_DIRECTORIES(${BUILD_PATH}/install/lib/)

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
    ${SWSS_PATH}/orchagent/pfcwdorch.cpp
    ${SWSS_PATH}/orchagent/pfcactionhandler.cpp
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

# TODO: Link to lib build by autoconfig
# TODO: Just can see source code (group), set as HEADER_FILE_ONLY
add_library(orchagent
    ${ORCH_SRCS}
    ${ORCH_HEADERS}
)

target_include_directories(orchagent PUBLIC
    ${SWSS_PATH}/orchagent/
    ${SAIREDIS_PATH}/SAI/inc/
    ${SAIREDIS_PATH}/SAI/meta/
    ${SAIREDIS_PATH}/lib/inc/
    ${BUILD_PATH}/install/include/ 
)

target_link_libraries(orchagent
    swsscommon sairedis saimetadata jansson
)

#################################################################

## test target ##################################################

configure_file (
    tests/spec_auto_config.h.in
    ${PROJECT_BINARY_DIR}/tests/spec_auto_config.h
)

file(GLOB TESTS
    main.cpp
    ${SWSS_PATH}/tests/fake_main.cpp
    ${SWSS_PATH}/tests/aclorch_ut.cpp
    tests/copporch_ut.cpp
    tests/qosorch_ut.cpp
    ${SWSS_PATH}/tests/saispy_ut.cpp
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

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "tests.out")
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR}/tests)
target_link_libraries(${PROJECT_NAME} ${GTEST_LIBRARIES})
target_link_libraries(${PROJECT_NAME} pthread hiredis swsscommon saimeta saimetadata orchagent saivs sairedis)

add_executable(syncd-tests
    syncd-tests/main.cpp

    syncd-tests/syncd_tests.cpp

    syncd-tests/sai_redis_metadata.cpp
    syncd-tests/sai_vs_metadata.cpp

	# ${SAIREDIS_PATH}/syncd/syncd.cpp
    syncd-tests/mock_syncd.cpp
	# ${SAIREDIS_PATH}/syncd/syncd_saiswitch.cpp
    syncd-tests/mock_syncd_saiswitch.cpp
	# ${SAIREDIS_PATH}/syncd/syncd_hard_reinit.cpp
    syncd-tests/mock_syncd_hard_reinit.cpp
	# ${SAIREDIS_PATH}/syncd/syncd_notifications.cpp
    syncd-tests/mock_syncd_notifications.cpp
	# ${SAIREDIS_PATH}/syncd/syncd_applyview.cpp
    syncd-tests/mock_syncd_applyview.cpp
	# ${SAIREDIS_PATH}/syncd/syncd_flex_counter.cpp
    syncd-tests/mock_syncd_flex_counter.cpp

    tests/fake_main.DEPRECATED.cpp

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
    # ${SWSS_PATH}/orchagent/saihelper.cpp
    syncd-tests/mock_saihelper.cpp
    ${SWSS_PATH}/orchagent/switchorch.cpp
    ${SWSS_PATH}/orchagent/pfcwdorch.cpp
    ${SWSS_PATH}/orchagent/pfcactionhandler.cpp
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

target_include_directories(syncd-tests PRIVATE 
    ${PROJECT_BINARY_DIR}/tests
    ${SWSS_PATH}/orchagent
    ${SAIREDIS_PATH}/SAI/inc
    ${SAIREDIS_PATH}/SAI/meta
    ${SAIREDIS_PATH}/lib/inc
    ${SAIREDIS_PATH}/syncd
    ${BUILD_PATH}/install/include
)

target_link_libraries(syncd-tests ${GTEST_LIBRARIES} pthread nl-3 nl-genl-3 nl-route-3 hiredis swsscommon saimeta saimetadata)

add_executable(cv-tests
    syncd-tests/condition_variable_test.cpp
)

target_link_libraries(cv-tests pthread)

## make test_coverage ###########################################
set(COVERAGE_LCOV_EXCLUDES
    "*/c++/*"
    "*/gtest/*"
    "*/sonic-swss-dev/main.cpp"
    "*/sonic-swss-common/common/*"
    "*/sonic-swss-dev/tests/*"
)

add_test(tests.out tests.out)
SETUP_TARGET_FOR_COVERAGE_LCOV(
    NAME test_coverage
    EXECUTABLE ctest
    DEPENDENCIES ${PROJECT_NAME}
    LCOV_ARGS
)
