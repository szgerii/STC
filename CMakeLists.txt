cmake_minimum_required(VERSION 3.13)
project(ShaderTranspilerCore)

# make sure julia executable is available
find_program(JULIA_EXEC julia)
if (NOT JULIA_EXEC)
    message(FATAL_ERROR "Couldn't locate Julia executable (needed for locating libjulia). Please ensure Julia is installed and added to PATH.")
endif()

# find and include Julia lib and headers
execute_process(
    COMMAND ${JULIA_EXEC} -e "print(normpath(joinpath(Sys.BINDIR, \"..\", \"include\", \"julia\")))"
    OUTPUT_VARIABLE JULIA_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${JULIA_EXEC} -e "using Libdl; print(Libdl.dlpath(\"libjulia\"))"
    OUTPUT_VARIABLE JULIA_LIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# on windows we can't link the dll itself
# on linux and mac, the file pointed to by libjulia should work
if (WIN32)
    get_filename_component(JULIA_BINDIR ${JULIA_LIB} DIRECTORY)
    set(JULIA_LIBDIR "${JULIA_BINDIR}/../lib")
    get_filename_component(JULIA_LIBDIR ${JULIA_LIBDIR} ABSOLUTE)

    set(JULIA_LIB "${JULIA_LIBDIR}/libjulia.dll.a")
    if (NOT EXISTS "${JULIA_LIB}")
        set(JULIA_LIB "${JULIA_LIBDIR}/julia.lib")
    endif()

    if (NOT EXISTS "${JULIA_LIB}")
        message(FATAL_ERROR "Couldn't locate Julia library (libjulia.dll.a or julia.lib)")
    endif()
endif()

message(STATUS "Found Julia include dir: ${JULIA_INCLUDE_DIR}")
message(STATUS "Found Julia library: ${JULIA_LIB}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/lib)

file(GLOB_RECURSE CPP_SOURCE CONFIGURE_DEPENDS "src/*.cpp" "include/*.h" "sandbox/*.cpp" "sandbox/*.h")

# ccache

find_program(CCACHE_EXEC ccache)
if (CCACHE_EXEC)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXEC}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_EXEC}")

    message(STATUS "ccache found and enabled for compiler caching")
else()
    message(WARNING "ccache not found, compiler caching won't be possible")
endif()

# clang-format

find_program(CLANG_FORMAT_EXEC NAMES clang-format)
if (CLANG_FORMAT_EXEC)
    # style checking
    add_custom_target(
        check-format
        COMMAND ${CLANG_FORMAT_EXEC}
                -style=file
                -n
                --Werror
                ${CPP_SOURCE}
        COMMENT "Checking C++ code style..."
    )

    # automatic style enforcement
    add_custom_target(
        fix-format
        COMMAND ${CLANG_FORMAT_EXEC}
                -style=file
                -i
                ${CPP_SOURCE}
        COMMENT "Fixing C++ code style..."
    )

    message(STATUS "clang-format found, check-format and fix-format targets enabled")
else()
    message(WARNING "clang-format could not be located, code style checking won't be possible.")
endif()

# clang-tidy

find_program(CLANG_TIDY_EXEC clang-tidy)
if (CLANG_TIDY_EXEC)
    set(CLANG_TIDY_CMD "${CLANG_TIDY_EXEC}" "-extra-arg=-Wno-unknown-warning-option")    
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_CMD})
    
    message(STATUS "clang-tidy found and enabled")
else()
    message(WARNING "clang-tidy could not be located, static analysis won't be possible.")
endif()

add_library(CommonConfig INTERFACE)

# extra safety options
if (MSVC)
    target_compile_options(CommonConfig INTERFACE /W4)
else()
    target_compile_options(CommonConfig INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion
    )
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # enable asan (and force disable incompatible options on MSVC)
    if (MSVC)
        target_compile_options(CommonConfig INTERFACE /fsanitize=address)

        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/ZI" "/Zi" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

        target_link_options(CommonConfig INTERFACE "/INCREMENTAL:NO")
    elseif(UNIX)
        target_compile_options(CommonConfig INTERFACE -fsanitize=address,undefined)
        target_link_options(CommonConfig INTERFACE -fsanitize=address,undefined)
    endif()
endif()

set(LIB_SOURCE ${CPP_SOURCE})
list(FILTER LIB_SOURCE EXCLUDE REGEX "sandbox/.*")

add_library(ShaderTranspilerCore SHARED ${LIB_SOURCE})
target_link_libraries(ShaderTranspilerCore PRIVATE CommonConfig)

target_include_directories(ShaderTranspilerCore PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(ShaderTranspilerCore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(ShaderTranspilerCore PRIVATE ${JULIA_INCLUDE_DIR})

target_link_libraries(ShaderTranspilerCore PRIVATE ${JULIA_LIB})

target_compile_definitions(ShaderTranspilerCore PRIVATE JULIA_ENABLE_THREADING=1)

set_target_properties(ShaderTranspilerCore PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY                ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY                ${LIB_OUTPUT_DIR}
    
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIB_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     ${LIB_OUTPUT_DIR}

    ARCHIVE_OUTPUT_DIRECTORY_DEBUG          ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE        ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIB_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     ${LIB_OUTPUT_DIR}
)

# sandbox targets
# for trying out and playing around with the library
# cpp files prefixed with run_ are treated as separate targets
# other cpp files can be used for includes and such

file(GLOB SANDBOX_SOURCE "sandbox/run_*.cpp")

foreach (SBOX_FILE ${SANDBOX_SOURCE})
    get_filename_component(FNAME ${SBOX_FILE} NAME_WE)

    add_executable(${FNAME} ${SBOX_FILE})

    target_link_libraries(${FNAME} PRIVATE ShaderTranspilerCore CommonConfig)

    target_include_directories(${FNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sandbox)
    target_include_directories(${FNAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # copy lib next to executable
    add_custom_command(TARGET ${FNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ShaderTranspilerCore>
        $<TARGET_FILE_DIR:${FNAME}>
    )
endforeach()

