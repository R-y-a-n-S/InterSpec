cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project( InterSpecTests )

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

if(MSVC)
  option(${PROJECT_NAME}_USE_MSVC_MultiThreadDLL "Use dynamically-link runtime library." OFF)

  if( ${PROJECT_NAME}_USE_MSVC_MultiThreadDLL)
    set(Boost_USE_STATIC_RUNTIME OFF)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  else()
    set(Boost_USE_STATIC_RUNTIME ON)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  endif()
  message(STATUS "Using CMAKE_MSVC_RUNTIME_LIBRARY ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()

enable_testing()


#if (WIN32)
#  add_definitions( -DBOOST_ALL_NO_LIB )
#  add_definitions( -DBOOST_ALL_DYN_LINK )
#endif()
#if(BUILD_AS_UNIT_TEST_SUITE)
#    target_link_libraries(
#        InterSpecLib
#        PUBLIC
#        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
#    )
#endif(BUILD_AS_UNIT_TEST_SUITE)


set( BUILD_AS_UNIT_TEST_SUITE ON CACHE BOOL "" )
set( INCLUDE_ANALYSIS_TEST_SUITE ON CACHE BOOL "" )
set( BUILD_AS_OFFLINE_ANALYSIS_TEST_SUITE ON CACHE BOOL "")


set(BUILD_AS_LOCAL_SERVER OFF CACHE BOOL "")
set(USE_DB_TO_STORE_SPECTRA ON CACHE BOOL "")
set(PERFORM_DEVELOPER_CHECKS ON CACHE BOOL "")
set(SpecUtils_ENABLE_EQUALITY_CHECKS ON CACHE BOOL "")
set(USE_SPECRUM_FILE_QUERY_WIDGET ON CACHE BOOL "")

# We dont currently export all functions tested when making a DLL for LibInterSpec, 
#  so we'll disable batch analysis, so a static library will be made.
set(USE_BATCH_TOOLS OFF CACHE INTERNAL "")

set(TEST_SUITE_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test_data"
    CACHE STRING
    "Path to directory that contains the \"analysis_tests\" directory for saving N42 test states."
)

add_subdirectory( ${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/LibInterSpec )

# Set the common arguments that are interpreted by boost unit_test
#  Note: not using quotes in next line; if we do, then all arguments will be passed as a quoted
#        single argument, which isnt what we want.
set( BOOST_TEST_CL_ARGS --log_level=test_suite --catch_system_error=yes --report_level=detailed )

# The arguments passed to our code, so the code can know where static nuclear data is, as well spectra and such
set( DATA_DIR_ARGS --datadir=${CMAKE_CURRENT_SOURCE_DIR}/../../data --testfiledir=${CMAKE_CURRENT_SOURCE_DIR}/test_data )

add_executable( test_PhysicalUnits test_PhysicalUnits.cpp )
target_link_libraries( test_PhysicalUnits PRIVATE InterSpecLib )
add_test( NAME PhysicalUnits
  COMMAND $<TARGET_FILE:test_PhysicalUnits> ${BOOST_TEST_CL_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable( test_PhysicalUnitsLocalized test_PhysicalUnitsLocalized.cpp )
target_link_libraries( test_PhysicalUnitsLocalized PRIVATE InterSpecLib )
add_test( NAME test_PhysicalUnitsLocalized
  COMMAND $<TARGET_FILE:test_PhysicalUnitsLocalized> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


add_executable( test_findCharacteristics test_findCharacteristics.cpp )
target_link_libraries( test_findCharacteristics PRIVATE InterSpecLib )
add_test( NAME FindCharacteristicNuclides
  COMMAND $<TARGET_FILE:test_findCharacteristics> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


add_executable( test_nuclideSearch test_nuclideSearch.cpp )
target_link_libraries( test_nuclideSearch PRIVATE InterSpecLib )
add_test( NAME NuclideSearch
  COMMAND $<TARGET_FILE:test_nuclideSearch> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


add_executable( test_RefLines test_RefLines.cpp )
target_link_libraries( test_RefLines PRIVATE InterSpecLib )
add_test( NAME TesRefLines
  COMMAND $<TARGET_FILE:test_RefLines> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


add_executable( test_DoseCalc test_DoseCalc.cpp )
target_link_libraries( test_DoseCalc PRIVATE InterSpecLib )
add_test( NAME TDoseCalc
  COMMAND $<TARGET_FILE:test_DoseCalc> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


add_executable( test_RelEffManual test_RelEffManual.cpp )
target_link_libraries( test_RelEffManual PRIVATE InterSpecLib )
add_test( NAME TestRelEffManual
  COMMAND $<TARGET_FILE:test_RelEffManual> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable( test_RelEffAuto test_RelEffAuto.cpp )
target_link_libraries( test_RelEffAuto PRIVATE InterSpecLib )
add_test( NAME TestRelEffAuto
  COMMAND $<TARGET_FILE:test_RelEffAuto> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable( test_PeakDists test_PeakDists.cpp )
target_link_libraries( test_PeakDists PRIVATE InterSpecLib )
add_test( NAME TPeakDists
  COMMAND $<TARGET_FILE:test_PeakDists> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


add_executable( test_AppUtils test_AppUtils.cpp )
target_link_libraries( test_AppUtils PRIVATE InterSpecLib )
add_test( NAME TAppUtils
  COMMAND $<TARGET_FILE:test_AppUtils> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable( test_ShieldingSourceFitCalc test_ShieldingSourceFitCalc.cpp )
target_link_libraries( test_ShieldingSourceFitCalc PRIVATE InterSpecLib )
add_test( NAME TShieldingSourceFitCalc
  COMMAND $<TARGET_FILE:test_ShieldingSourceFitCalc> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable( test_DetectionLimit test_DetectionLimit.cpp )
target_link_libraries( test_DetectionLimit PRIVATE InterSpecLib )
add_test( NAME test_DetectionLimit
  COMMAND $<TARGET_FILE:test_DetectionLimit> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable( test_MakeDrfSrcDef test_MakeDrfSrcDef.cpp )
target_link_libraries( test_MakeDrfSrcDef PRIVATE InterSpecLib )
add_test( NAME TMakeDrfSrcDef
  COMMAND $<TARGET_FILE:test_MakeDrfSrcDef> ${BOOST_TEST_CL_ARGS} -- ${DATA_DIR_ARGS}
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)


# ./test_offlineAnalysis.cpp
#target_link_libraries(test_offlineAnalysis PUBLIC 
#            debug "${Wt_TEST_DEBUG_LIBRARY}"
#            optimized "${Wt_TEST_LIBRARY}"
#)


# For some reason in Xcode, the RUN_TESTS target doesnt cause the executables to be built if there
# is code changes - it will just run the already compiled tests.  To get around this we will define
# a target that depends on ALL_BUILD and RUN_TESTS.
# Note however, that Xcode wont show result of tests - it only will display an error if a test fails
if( CMAKE_GENERATOR STREQUAL "Xcode" )
  add_custom_target( build_then_run_tests DEPENDS ALL_BUILD RUN_TESTS )
endif( CMAKE_GENERATOR STREQUAL "Xcode" )
