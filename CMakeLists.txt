cmake_minimum_required(VERSION 3.1)

project(SICXEAssembler)
set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${VERSION})

include(GNUInstallDirs)


# Portable application
option(BUILD_STATIC "Link executable statically" OFF)


# Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

    # Enable warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    # Visibility
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections")

    if (${BUILD_STATIC})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
    endif()
endif()


# sicasm
set(SICASM_SOURCES
    main.cpp
    assembler.cpp
    instructions.cpp
)

add_executable(sicasm ${SICASM_SOURCES})

if(NOT MSVC)
    set_target_properties(sicasm PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED 1
    )
endif()

install(
    TARGETS sicasm
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)


# CPack
set(CPACK_PACKAGE_NAME "sicasm")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SIC/XE Assembler")
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2;ZIP")

include(CPack)
