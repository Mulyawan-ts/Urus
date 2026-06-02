cmake_minimum_required(VERSION 3.10)
project(UrusCompiler VERSION 0.1.0 LANGUAGES C)

set(PROJECT_VERSION_TAGS "")
set(CMAKE_C_STANDARD 11) # -std=c11
set(CMAKE_C_STANDARD_REQUIRED ON)

# The embedded runtime (runtime/urus_runtime.h) relies on the
# __attribute__((cleanup)) extension from GCC/Clang for RAII. MSVC
# native silently no-ops the attribute, so user programs built with
# the resulting compiler would leak every heap-owning local at scope
# exit with no diagnostic. Refuse to configure here so the failure is
# obvious at setup time rather than at run time of a stranger's
# program. clang-cl identifies itself as Clang and is accepted.
if(MSVC AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "Urus v0.1.0 cannot be built with native MSVC: the runtime "
        "requires __attribute__((cleanup)) for RAII, which MSVC does "
        "not implement. Use clang-cl, MSYS2 GCC, or WSL. A portable "
        "explicit-drop codegen path is tracked as a follow-up.")
endif()

configure_file(config.h.in config.h)

if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
else()
    add_compile_options(-Wall -Wextra -g)
endif()

set(URUS_RUNTIME_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/runtime/urus_runtime.h")
set(URUS_RUNTIME_EMBEDDED "${CMAKE_CURRENT_BINARY_DIR}/urus_runtime.c")

# Generate the embedded runtime exactly once per build.
#
# Previously, urus_runtime.c was listed directly in the source lists of both
# the `urusc` and `urusc-lsp` executables. CMake then attached the
# add_custom_command that generates the file to *both* targets, and parallel
# `make -j` builds invoked the embed-string.cmake script twice concurrently.
# Because the script does file(WRITE ...) followed by file(APPEND ...), a
# losing-the-race second writer could observe the post-WRITE / pre-APPEND
# state and end up appending the *_len line a second time, producing a file
# with two `urus_runtime_header_data_len` definitions and breaking the link.
#
# Fix: compile urus_runtime.c into an OBJECT library and have each executable
# depend on the resulting object. The custom command is attached to exactly
# one target, so the generator runs exactly once even under -j.
add_custom_command(
    OUTPUT "${URUS_RUNTIME_EMBEDDED}"
    COMMAND ${CMAKE_COMMAND}
            -DINPUT_FILE=${URUS_RUNTIME_HEADER}
            -DOUTPUT_FILE=${URUS_RUNTIME_EMBEDDED}
            -DVARIABLE_NAME=urus_runtime_header_data
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed-string.cmake"
    DEPENDS "${URUS_RUNTIME_HEADER}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed-string.cmake"
    VERBATIM
)

add_library(urus_runtime_obj OBJECT "${URUS_RUNTIME_EMBEDDED}")

# source
set(SOURCES
    urusc.c
    lexer.c
    ast.c
    parser.c
    misc.c
    error.c
    sema.c
    builtins.c
    codegen.c
    preprocess.c
    pkg.c
)

add_executable(urusc ${SOURCES} $<TARGET_OBJECTS:urus_runtime_obj>)

target_include_directories(urusc PRIVATE "${CMAKE_CURRENT_BINARY_DIR}") # -I<BUILD DIRECTORIES> (example for config.h by config.h.in)

# LSP server (shares compiler frontend sources)
set(LSP_SHARED_SOURCES
    lexer.c
    ast.c
    parser.c
    misc.c
    error.c
    sema.c
    builtins.c
    preprocess.c
)
add_executable(urusc-lsp lsp.c ${LSP_SHARED_SOURCES} $<TARGET_OBJECTS:urus_runtime_obj>)
target_include_directories(urusc-lsp PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
install(TARGETS urusc-lsp DESTINATION bin)

# -lm for non-windows
if(UNIX AND NOT APPLE)
    target_link_libraries(urusc PRIVATE m)
endif()

# Install urusc
install(TARGETS urusc DESTINATION bin)

# Install stdlib/* to a relative `lib/urusc` under the install prefix.
#
# Historically this branched per-platform with absolute paths
# ("/usr/local/lib/urusc", "Program Files/Urusc/lib", ...). That hurt
# CPack: CPack always rewrites DESTINATION relative to the *bundle*
# root, so an absolute "/usr/local/lib/urusc" inside a .deb still
# means /usr/local/lib/urusc (fine), but on Windows the literal
# "Program Files/Urusc/lib" was treated as a path *under* the install
# prefix, producing C:/Program Files/Urus/Program Files/Urusc/lib —
# nested junk.
#
# A relative DESTINATION ("lib/urusc") plus an explicit CMAKE_INSTALL_PREFIX
# default per platform gets us both behaviours correctly:
#   * `sudo cmake --install build`        — root-level install where the
#                                            user already set the prefix
#                                            (or distro defaults to /usr/local).
#   * `cmake --build build --target package` — CPack lays the files out
#                                              under bin/ + lib/urusc/
#                                              inside the bundle.

if(NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{PREFIX})
        set(CMAKE_INSTALL_PREFIX "$ENV{PREFIX}" CACHE PATH "" FORCE)
    endif()
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/stdlib/"
        DESTINATION "lib/urusc"
        FILES_MATCHING PATTERN "*.urus")

# CTest integration
enable_testing()

set(TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tests")

# Valid compilation tests (should pass)
file(GLOB VALID_TESTS "${TESTS_DIR}/valid/*.urus")
foreach(test_file ${VALID_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME valid_${test_name} COMMAND urusc ${test_file} --emit-c)
endforeach()

# Invalid compilation tests (should fail)
file(GLOB INVALID_TESTS "${TESTS_DIR}/invalid/*.urus")
foreach(test_file ${INVALID_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME invalid_${test_name} COMMAND urusc ${test_file} --emit-c)
    set_tests_properties(invalid_${test_name} PROPERTIES WILL_FAIL TRUE)
endforeach()

# Determinism tests: re-emitting the C source for the same input must
# produce byte-identical output. Covers a representative slice rather
# than every run test — the goal is to catch hash/iteration-order or
# fresh-RNG leaks into codegen, not to multiply CI cost.
set(DETERMINISM_FIXTURES
    hello.urus
    tuples.urus
    closures.urus
    generics.urus
    nested_tuple_typedef.urus
    many_lambdas.urus
)
foreach(fixture ${DETERMINISM_FIXTURES})
    set(fixture_path "${TESTS_DIR}/run/${fixture}")
    if(EXISTS ${fixture_path})
        get_filename_component(fixture_name ${fixture} NAME_WE)
        add_test(NAME determinism_${fixture_name}
            COMMAND ${CMAKE_COMMAND}
                -DURUSC=$<TARGET_FILE:urusc>
                -DFILE=${fixture_path}
                -DWORKING_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -P "${TESTS_DIR}/determinism.cmake"
        )
    endif()
endforeach()

# Run tests (compile, run, check output)
file(GLOB RUN_TESTS "${TESTS_DIR}/run/*.urus")
foreach(test_file ${RUN_TESTS})
    get_filename_component(test_name ${test_file} NAME_WE)
    set(expected_file "${TESTS_DIR}/run/expected/${test_name}.expected")

    if(EXISTS ${expected_file})
        add_test(NAME run_${test_name}
            COMMAND ${CMAKE_COMMAND}
                -DURUSC=$<TARGET_FILE:urusc>
                -DFILE=${test_file}
                -DEXPECTED=${expected_file}
                -DINCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/include
                -DWORKING_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                -P "${TESTS_DIR}/run_tests.cmake"
        )
    endif()
endforeach()

# ----------------------------------------------------------------------
# Packaging — CPack-driven installers per platform.
# ----------------------------------------------------------------------
#
# Issue #183 asked for a Windows MSI / EXE installer so newcomers don't
# have to learn CMake before they can try the language. CPack is the
# native CMake answer for this: configure the project metadata here,
# then `cmake --build build --target package` produces an installer
# bundle of whatever generator we ask for.
#
# We deliberately wire CPack for *every* platform (not just Windows):
#   * Windows  — NSIS (.exe). Lightweight, single-file, no MSI tooling
#                required to build. Adds Start Menu shortcut and an
#                'Add to PATH' option at install time.
#   * macOS    — productbuild (.pkg). The graphical installer most macOS
#                users already recognise.
#   * Linux    — DEB + RPM. Picked up by the matching distro tooling
#                without extra work.
#   * Source   — TGZ + ZIP. Useful for offline distribution.
#
# CPack reads the install(TARGETS ...) and install(DIRECTORY ...) rules
# above to decide what to put in the bundle — `urusc` and the stdlib
# already register through those, so no separate manifest is needed.

set(CPACK_PACKAGE_NAME "Urus")
set(CPACK_PACKAGE_VENDOR "Urus Foundation")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "URUS — statically-typed systems language that transpiles to C11")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Urus-Foundation/Urus")
set(CPACK_PACKAGE_CONTACT "rasyaandrean@outlook.co.id")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")
set(CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")

# Apple's productbuild is fussy about resource-file extensions: it
# only accepts .rtfd / .rtf / .html / .txt for the installer's
# License / ReadMe / Welcome panes. Our repo ships LICENSE (no
# extension) and README.md (.md), both of which it rejects. On
# macOS we materialise .txt copies in the build dir and point CPack
# at those — leaving the source-of-truth files untouched.
if(APPLE)
    set(_PB_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
    set(_PB_README  "${CMAKE_CURRENT_BINARY_DIR}/README.txt")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE"
                   "${_PB_LICENSE}" COPYONLY)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/../README.md"
                   "${_PB_README}"  COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${_PB_LICENSE}")
    set(CPACK_RESOURCE_FILE_README  "${_PB_README}")
endif()

# Source-package generators apply on every host.
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    "/\\\\.git/"
    "/build/"
    "/cmake-build-.*/"
    "/\\\\.vs/"
    "/\\\\.idea/"
    "/\\\\.vscode/"
    "\\\\.o$"
    "\\\\.obj$"
)

if(WIN32)
    # NSIS is what `choco install nsis` / Scoop ships; both the GitHub
    # Actions windows-latest image and a fresh developer machine can
    # produce an .exe with no extra setup.
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Urus Compiler")
    set(CPACK_NSIS_PACKAGE_NAME "Urus")
    set(CPACK_NSIS_HELP_LINK    "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_URL_INFO_ABOUT "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_NSIS_CONTACT      "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_MODIFY_PATH ON)   # offers an 'Add Urus to PATH' checkbox
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Urus")

    # Start Menu shortcut.
    #
    # urusc itself is a CLI tool — clicking a shortcut to urusc.exe would
    # flash a console window and close immediately, which is worse than
    # no shortcut. Instead we install a small launcher script that opens
    # an interactive cmd.exe with PATH and URUSCPATH already pointing at
    # the install prefix. The shortcut targets that script.
    #
    # CPACK_PACKAGE_EXECUTABLES is a flat list of (target_name, label)
    # pairs. NSIS looks for `bin/<target_name>.exe`; we ship our
    # launcher with a .cmd extension, so we register an extra
    # CPACK_NSIS_CREATE_ICONS_EXTRA section that creates the shortcut
    # by hand and CPACK_NSIS_DELETE_ICONS_EXTRA that removes it on
    # uninstall. Same pattern Chocolatey and a few others use.
    install(PROGRAMS
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/urus-prompt.cmd"
            DESTINATION bin)

    set(CPACK_NSIS_MENU_LINKS
        "bin\\\\urus-prompt.cmd" "Urus Compiler Prompt"
        "${CPACK_PACKAGE_HOMEPAGE_URL}" "Urus on GitHub")

    # Optional desktop shortcut. The NSIS generator can offer this as a
    # tick-box at install time when CPACK_NSIS_CREATE_ICONS_EXTRA writes
    # the shortcut and CPACK_NSIS_DELETE_ICONS_EXTRA removes it.
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\Urus Compiler Prompt.lnk' '$INSTDIR\\\\bin\\\\urus-prompt.cmd'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\Urus Compiler Prompt.lnk'")
elseif(APPLE)
    set(CPACK_GENERATOR "productbuild;TGZ")
elseif(UNIX)
    # DEB + RPM cover Debian/Ubuntu and Fedora/RHEL/openSUSE families.
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "gcc")
    set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
    set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_RPM_PACKAGE_REQUIRES "gcc")
endif()

include(CPack)
