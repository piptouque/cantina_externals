cmake_minimum_required(VERSION 3.6)

project(cantina_tilde)


# see: https://cmake.org/cmake/help/v2.9/manual/cmake-variables.7.html
message("CMAKE_VERSION          : " ${CMAKE_VERSION})
message("CMAKE_SYSTEM_NAME      : " ${CMAKE_SYSTEM_NAME})
message("CMAKE_SYSTEM           : " ${CMAKE_SYSTEM})
message("CMAKE_SYSTEM_PROCESSOR : " ${CMAKE_SYSTEM_PROCESSOR})

set(THIRDPARTY_DIR ${PROJECT_SOURCE_DIR}/third-party)

set(CANTINA_TILDE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/source)
set(CANTINA_TILDE_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(CANTINA_TILDE_EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/externals)

# spell out the directories
message("CMAKE_BINARY_DIR       : " ${CMAKE_BINARY_DIR})
message("CMAKE_SOURCE_DIR       : " ${CMAKE_SOURCE_DIR})
message("CMAKE_HOME_DIRECTORY   : " ${CMAKE_HOME_DIRECTORY})
message("PROJECT_BINARY_DIR     : " ${PROJECT_BINARY_DIR})
message("PROJECT_SOURCE_DIR     : " ${PROJECT_SOURCE_DIR})
message("PROJECT_LIB_DIR        : " ${PROJECT_LIB_DIR})
message("GIT_SUBMODULE_DIR:    : " ${GIT_SUBMODULE_DIR})



#cantina
set(CANTINA_DIR ${PROJECT_SOURCE_DIR}/cantina)
add_subdirectory(${CANTINA_DIR})

set(CMAKE_CXX_STANDARD ${CANTINA_CXX_STANDARD})
set(CMAKE_CXX_FLAGS ${CANTINA_TILDE_CXX_FLAGS})

include_directories(
        ${CANTINA_TILDE_INCLUDE_DIR}
        ${CANTINA_INCLUDE_DIRS}
)

# variable used in pd.build
set(PD_DIR ${GIT_SUBMODULE_DIR}/pure-data)

set(CMAKE_SUPPRESS_REGENERATION true)
# who cares about mac anyway ?
set(CMAKE_MACOSX_RPATH off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)
set(CMAKE_OSC_ARCHITECTURES "i386;x86_64")

# pd.cmake module
include(${THIRDPARTY_DIR}/pd.build/pd.cmake)
set_pd_sources(${THIRDPARTY_DIR}/pure-data/src)
set_pd_external_path(${CANTINA_TILDE_EXTERNAL_DIR})
set(CANTINA_TILDE_FILES
        # external
        ${CANTINA_TILDE_SOURCE_DIR}/cantina~.cpp
        ${CANTINA_TILDE_INCLUDE_DIR}/cantina~.hpp
        ${CANTINA_TILDE_INCLUDE_DIR}/cant/impl.hpp

        ${CANTINA_FILES}
        )

add_pd_external(cantina_tilde cantina~ ${CANTINA_TILDE_FILES})

# LINKIN'
target_link_libraries(cantina_tilde
        ${CANTINA_LIBRARIES}
        )
