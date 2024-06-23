cmake_minimum_required(VERSION 3.5)

SET(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_VERBOSE_MAKEFILE ON)

set( SATURN_COMMON $ENV{SATURN_COMMON} )
set( SATURN_CMAKE $ENV{SATURN_CMAKE} )
set( SATURN_ROOT $ENV{SATURN_ROOT} )

set( SEGALIB $ENV{SATURN_SBL}/segalib )
set( SEGASGL $ENV{SATURN_SGL} )

set( TARGET_NAME flashback_saturn )
set( TARGET_PATH flashback_saturn )

# remap ABS.TXT, BIB.TXT and CPY.TXT to the project's ones
set( ENV{SATURN_CD} "${PROJECT_SOURCE_DIR}/saturnport/" )

project( ${TARGET_NAME} VERSION 0.1.9
                  DESCRIPTION "Du coup flashback"
                  LANGUAGES C CXX ASM)

set(as_flags )

set(c_flags
        -fpermissive
        -fno-lto
        -Os
        -nostdlib
        -m2
     )

set(cxx_flags
        -fno-lto
        -Os
        -fno-rtti
        -fno-exceptions
        -m2
     )

set(SOURCES
      ${PROJECT_SOURCE_DIR}/src/gfs_wrap.cpp
      ${PROJECT_SOURCE_DIR}/src/collision.cpp
      ${PROJECT_SOURCE_DIR}/src/cutscene.cpp
      ${PROJECT_SOURCE_DIR}/src/file.cpp
      ${PROJECT_SOURCE_DIR}/src/game.cpp
      ${PROJECT_SOURCE_DIR}/src/graphics.cpp
      ${PROJECT_SOURCE_DIR}/src/main.cpp
      ${PROJECT_SOURCE_DIR}/src/menu.cpp
      ${PROJECT_SOURCE_DIR}/src/mixer.cpp
      ${PROJECT_SOURCE_DIR}/src/piege.cpp
      ${PROJECT_SOURCE_DIR}/src/unpack.cpp
      ${PROJECT_SOURCE_DIR}/src/resource.cpp
      ${PROJECT_SOURCE_DIR}/src/resource_mac.cpp
      ${PROJECT_SOURCE_DIR}/src/resource_aba.cpp
      ${PROJECT_SOURCE_DIR}/src/decode_mac.cpp
      ${PROJECT_SOURCE_DIR}/src/sfx_player.cpp
      ${PROJECT_SOURCE_DIR}/src/staticres.cpp
      ${PROJECT_SOURCE_DIR}/src/systemstub_sdl.cpp
      ${PROJECT_SOURCE_DIR}/src/util.cpp
      ${PROJECT_SOURCE_DIR}/src/video.cpp
      ${PROJECT_SOURCE_DIR}/src/sat_mem_checker.cpp
      ${PROJECT_SOURCE_DIR}/src/lz.cpp
      ${PROJECT_SOURCE_DIR}/src/cinit.cpp
      ${PROJECT_SOURCE_DIR}/src/workarea.c
      ${PROJECT_SOURCE_DIR}/src/low.s
     )

set(CD_FILES
      0.bin
      FLASHBCK.RSR
      SDDRVS.TSK
      )

add_compile_definitions(
                        MODEL_S
                        _SH
                        _SATURNPORT_
                        SYS_BIG_ENDIAN
                        BYPASS_PROTECTION
                         )

add_executable( ${TARGET_NAME}.elf ${SOURCES} )

target_include_directories(
        ${TARGET_NAME}.elf PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${SEGALIB}/include
        ${SEGASGL}/include
        )

target_compile_options( ${TARGET_NAME}.elf PUBLIC $<$<COMPILE_LANGUAGE:CXX>:${cxx_flags}>
                                                $<$<COMPILE_LANGUAGE:C>:${c_flags}> )

target_link_options(
    ${TARGET_NAME}.elf PUBLIC
    "SHELL:-Xlinker -Map -Xlinker ${TARGET_NAME}.map"
    "SHELL:-Xlinker --strip-debug"
    "SHELL:-Xlinker -e -Xlinker ___Start"
    "SHELL:-Xlinker -fno-lto"
    "SHELL:-m2 -nostartfiles -fpermissive -fno-builtin"
    "SHELL:-T${PROJECT_SOURCE_DIR}/saturnport/saturn.lnk"
)

add_custom_target( run_${TARGET_NAME} ALL DEPENDS
                    ${TARGET_NAME}.elf
                    ${TARGET_NAME}.bin)

add_custom_command( OUTPUT ${TARGET_NAME}.bin
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMAND ${CMAKE_OBJCOPY}
                    ARGS -O binary ${TARGET_NAME}.elf ${TARGET_NAME}.bin )

target_link_libraries( ${TARGET_NAME}.elf PUBLIC stdc++ )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGASGL}/lib/libsgl.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${PROJECT_SOURCE_DIR}/saturnport/sega_gfs2.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_csh_sgl.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_per_vbt.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_mem.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_dma.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_snd.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_sys.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_cdc.a )
target_link_libraries( ${TARGET_NAME}.elf PUBLIC ${SEGALIB}/lib/sega_int.a )

install(TARGETS ${TARGET_NAME}.elf
        DESTINATION ${CMAKE_INSTALL_PREFIX} )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
              ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
        DESTINATION ${CMAKE_INSTALL_PREFIX} )

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src")
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
          DESTINATION ${CMAKE_INSTALL_PREFIX}
          PATTERN ".git" EXCLUDE )
else()
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
          DESTINATION ${CMAKE_INSTALL_PREFIX}/src
          PATTERN ".git" EXCLUDE
          PATTERN "src" EXCLUDE )
endif()

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
          DESTINATION ${CMAKE_INSTALL_PREFIX}/CD
          RENAME 0.bin )

install(FILES ${PROJECT_SOURCE_DIR}/saturnport/IP.BIN
              ${PROJECT_SOURCE_DIR}/saturnport/SDDRVS.TSK
                DESTINATION ${CMAKE_INSTALL_PREFIX}/CD )

install(FILES ${PROJECT_SOURCE_DIR}/data/FLASHBCK.RSR
                DESTINATION ${CMAKE_INSTALL_PREFIX}/CD )

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.cue
                DESTINATION ${CMAKE_INSTALL_PREFIX} )

list(TRANSFORM CD_FILES PREPEND "${CMAKE_INSTALL_PREFIX}/CD/")

# Not very nice, it is matching the pathspec file grammar (e.g. output=input syntax)
set(files_to_write)
foreach(ITR ${CD_FILES})  # ARGN holds all arguments to function after last named one
  if(ITR MATCHES "(.*)=(.*)")
    list(APPEND files_to_write "${CMAKE_MATCH_2}=${CMAKE_MATCH_1}")
  else()
      list(APPEND files_to_write ${ITR})
  endif()
endforeach()
list(TRANSFORM files_to_write APPEND "\\n" )

set (filename ${CMAKE_INSTALL_PREFIX}/CD/FILES.txt )

install(CODE "file(WRITE ${filename} ${files_to_write})" )

set(MAKE_CMD /opt/saturn/cd_resources/create_cd.sh
    "${TARGET_NAME}" "\"CD0001\"" "\"VBT\"" IP.BIN
    "${TARGET_NAME}.iso" "${CMAKE_INSTALL_PREFIX}/" )

install(CODE "execute_process(COMMAND
                                ${MAKE_CMD}
                                WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/CD)" )
