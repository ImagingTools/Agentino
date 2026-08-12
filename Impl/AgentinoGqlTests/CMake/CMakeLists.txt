project(AgentinoGqlTests LANGUAGES CXX)

set(ARXC_CONFIG "${PROJECT_SOURCE_DIR}/../../../Config/Agentino.awc")
set(ARXC_FILES "${PROJECT_SOURCE_DIR}/../${PROJECT_NAME}.acc")
set(ARXC_OUTDIR "${AUX_INCLUDE_DIR}")

include(${ACFDIR}/Config/CMake/CustomBuild.cmake)

find_package("Qt${QT_VERSION_MAJOR}" COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(TEST_SOURCES
	${PROJECT_SOURCE_DIR}/../Main.cpp
	${PROJECT_SOURCE_DIR}/../Tests/CAgentsGqlTest.cpp
	${PROJECT_SOURCE_DIR}/../Tests/CServicesGqlTest.cpp
	${PROJECT_SOURCE_DIR}/../Tests/CTopologyGqlTest.cpp
	${PROJECT_SOURCE_DIR}/../Tests/CServiceLogGqlTest.cpp
	${PROJECT_SOURCE_DIR}/../Tests/CServerServiceGqlTest.cpp
	${PROJECT_SOURCE_DIR}/../Tests/CAgentSettingsGqlTest.cpp
)

set(TEST_HEADERS
	${PROJECT_SOURCE_DIR}/../Tests/CAgentsGqlTest.h
	${PROJECT_SOURCE_DIR}/../Tests/CServicesGqlTest.h
	${PROJECT_SOURCE_DIR}/../Tests/CTopologyGqlTest.h
	${PROJECT_SOURCE_DIR}/../Tests/CServiceLogGqlTest.h
	${PROJECT_SOURCE_DIR}/../Tests/CServerServiceGqlTest.h
	${PROJECT_SOURCE_DIR}/../Tests/CAgentSettingsGqlTest.h
	${PROJECT_SOURCE_DIR}/../Tests/TestHelpers.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockServiceController.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockObjectCollection.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockServiceCompositeInfo.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockServiceManager.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockServiceInfo.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockAgentInfo.h
	${PROJECT_SOURCE_DIR}/../Mocks/CMockConnectionCollectionProvider.h
)

add_executable(${PROJECT_NAME} ${TEST_SOURCES} ${TEST_HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}/..
	${PROJECT_SOURCE_DIR}/../Tests
	${PROJECT_SOURCE_DIR}/../Mocks
)

# Link Agentino GQL libraries
target_link_libraries(${PROJECT_NAME} agentinosdl agentinogql agentgql agentinodata)

# Link ImtCore libraries
target_link_libraries(${PROJECT_NAME} imtservergql imtclientgql imtgql imtbase imtcom imtservice)

# Link ACF libraries
target_link_libraries(${PROJECT_NAME} icomp iser istd imod ilog i2d)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Test)
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::Network)
target_link_libraries(${PROJECT_NAME} Qt${QT_VERSION_MAJOR}::WebSockets)

include(${ACFDIR}/Config/CMake/AcfQt.cmake)
include(${ACFDIR}/Config/CMake/AcfStd.cmake)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)
