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