cmake_minimum_required(VERSION 3.20)

project(oracle C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/out")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${OUTPUT_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${OUTPUT_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${OUTPUT_DIR}")
endforeach()

if(WIN32)
    set(OCI_CANDIDATE_NAMES oci)
else()
    set(OCI_CANDIDATE_NAMES clntsh oci)
endif()

find_library(
    OCI_LIBRARY
    NAMES ${OCI_CANDIDATE_NAMES}
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}"
    NO_DEFAULT_PATH
)

if(NOT OCI_LIBRARY)
    find_library(OCI_LIBRARY NAMES ${OCI_CANDIDATE_NAMES})
endif()

if(NOT OCI_LIBRARY)
    message(FATAL_ERROR "OCI library was not found. Put it next to CMakeLists.txt (e.g. oci.lib or libclntsh.*), or install Oracle client libraries.")
endif()

add_library(oracle_shared SHARED oracle.c)
target_include_directories(oracle_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/header")
target_compile_definitions(oracle_shared PRIVATE RESTAPI_BUILD_DLL)
target_link_libraries(oracle_shared PUBLIC "${OCI_LIBRARY}")
set_target_properties(oracle_shared PROPERTIES OUTPUT_NAME oracle)

add_library(oracle_static STATIC oracle.c)
target_include_directories(oracle_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/header")
target_link_libraries(oracle_static PUBLIC "${OCI_LIBRARY}")
set_target_properties(oracle_static PROPERTIES OUTPUT_NAME oracle_static)

add_executable(example example.c)
target_include_directories(example PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/header")
target_link_libraries(example PRIVATE oracle_static)
