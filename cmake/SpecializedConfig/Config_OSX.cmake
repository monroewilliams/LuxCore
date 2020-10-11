###########################################################################
#
# Configuration ( Jens Verwiebe )
#
###########################################################################

#MESSAGE(STATUS "Using OSX Configuration settings")

# Allow for the location of OSX_DEPENDENCY_ROOT to be set from the command line
IF( NOT OSX_DEPENDENCY_ROOT )
  set(OSX_DEPENDENCY_ROOT ${CMAKE_SOURCE_DIR}/macos) # can be macos or usr/local for example
ENDIF()

MESSAGE(STATUS "OSX_DEPENDENCY_ROOT : " ${OSX_DEPENDENCY_ROOT})
set(OSX_SEARCH_PATH     ${OSX_DEPENDENCY_ROOT})

# Libs present in system ( /usr )
SET(SYS_LIBRARIES z )

#Find pyside2-uic so that .ui files rebuild pyside deps
execute_process(COMMAND python3 -c "import sys; print(sys.base_exec_prefix)" OUTPUT_VARIABLE PY_EXEC_PREFIX)
find_program(PYSIDE_UIC
             NAME pyside2-uic
             HINTS ${PY_EXEC_PREFIX}/bin/)


find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_CXX_LIBRARIES}")


#### Enable debug info generation
# Set a debug info format
set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
# Prevent debug info from being stripped from all the static library sub-target builds
set(CMAKE_XCODE_ATTRIBUTE_STRIP_INSTALLED_PRODUCT "NO")
# I think this is already happening when the build style is set to Debug or RelWithDebInfo
#SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
