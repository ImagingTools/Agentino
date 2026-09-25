cmake_minimum_required(VERSION 3.26)


if(NOT DEFINED AGENTINODIR)
	file(TO_CMAKE_PATH "$ENV{AGENTINODIR}" AGENTINODIR)
endif()

if(NOT DEFINED AGENTINODIR_BUILD)
	file(TO_CMAKE_PATH "$ENV{AGENTINODIR_BUILD}" AGENTINODIR_BUILD)
	if(AGENTINODIR_BUILD STREQUAL "")
		set(AGENTINODIR_BUILD ${AGENTINODIR})
	endif()
endif()

if(NOT DEFINED IMTCOREDIR)
	file(TO_CMAKE_PATH "$ENV{IMTCOREDIR}" IMTCOREDIR)
	if(IMTCOREDIR STREQUAL "")
		set(IMTCOREDIR ${AGENTINODIR}/../ImtCore)
	endif()
endif()

include(${IMTCOREDIR}/Config/CMake/ImtCoreEnv.cmake)

include_directories("${AGENTINODIR}/Sdl")
include_directories("${AGENTINODIR_BUILD}/AuxInclude/${TARGETNAME}/GeneratedFiles")
include_directories("${AGENTINODIR_BUILD}/AuxInclude/${TARGETNAME}")

if(NOT TARGET ImtCore::imtbase)
	# Discover Acf::/AcfSln::/IAcf::/ImtCore::
	set(ImtCore_DIR "${IMTCOREDIR_BUILD}/Lib/${CMAKE_BUILD_TYPE}_${TARGETNAME}/cmake")
	message(VERBOSE "AgentinoEnv find_package(ImtCore) from ${ImtCore_DIR}")
	find_package(ImtCore REQUIRED GLOBAL)
endif()

