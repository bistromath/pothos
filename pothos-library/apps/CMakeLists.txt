########################################################################
# Local includes
########################################################################
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${POTHOS_INCLUDE_DIRS})

########################################################################
# apps
########################################################################
list(APPEND SOURCES
    PothosUtil.cpp
    PothosUtilSelfTests.cpp
    PothosUtilPrintTree.cpp
    PothosUtilProxyServer.cpp
    PothosUtilLoadModule.cpp
    PothosUtilDocParse.cpp
)
add_executable(PothosUtil ${SOURCES})
target_link_libraries(PothosUtil Pothos ${POTHOS_LIBRARIES})
install(TARGETS PothosUtil
    RUNTIME DESTINATION bin COMPONENT pothos_runtime
)
