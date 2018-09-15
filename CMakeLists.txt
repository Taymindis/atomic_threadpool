cmake_minimum_required(VERSION 2.8)
project(athpool)

set(PROJNAME athpool)

IF (WIN32 OR MSVC OR MSYS OR MINGW OR APPLE)
message( FATAL_ERROR "for windows build, please open with visual studio project")
endif()

MESSAGE(STATUS "Unix Build ... ")

IF (NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)
SET(CMAKE_INSTALL_INCLUDEDIR /usr/local/include)
ENDIF(NOT DEFINED CMAKE_INSTALL_INCLUDEDIR)

include_directories(/usr/local/include /usr/include lfqueue)

file(GLOB_RECURSE sources at_thpool.h at_thpool.c lfqueue/lfqueue.h lfqueue/lfqueue.c)
file(GLOB_RECURSE example_test ${sources} threadpool_example.c)

add_executable(example_threadpool ${example_test})
target_link_libraries(example_threadpool PUBLIC pthread)


SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wstrict-prototypes -Wmissing-prototypes")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -pedantic")

###############################################################################
## packaging ##################################################################
###############################################################################
add_library(${PROJNAME}.static STATIC ${sources})
target_link_libraries(${PROJNAME}.static pthread)
set_target_properties(${PROJNAME}.static PROPERTIES OUTPUT_NAME ${PROJNAME})
add_library(${PROJNAME} SHARED ${sources})
target_link_libraries(${PROJNAME} pthread)


# link_directories(/usr/local/lib /usr/lib)

## For .a library
install(TARGETS ${PROJNAME}.static EXPORT ${PROJNAME}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

## For .so library
install(TARGETS ${PROJNAME} EXPORT ${PROJNAME}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    # RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
install(FILES at_thpool.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(CODE "execute_process(COMMAND ldconfig)")


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)


set(CPACK_PACKAGE_NAME "at_thpool")
set(CPACK_PACKAGE_VERSION "1.0.0")

# we don't want to split our program up into several things
set(CPACK_MONOLITHIC_INSTALL 1)

# This must be last
include(CPack)
