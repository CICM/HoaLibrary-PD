cmake_minimum_required(VERSION 2.8)

set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

include(pd.build/pd.cmake)
project(hoa)
set_pd_sources(${PROJECT_SOURCE_DIR}/pd/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/hoa/")

file(GLOB hoa_library_src ${PROJECT_SOURCE_DIR}/hoa.library/Sources/*.cpp ${PROJECT_SOURCE_DIR}/hoa.library/Sources/*.hpp)
source_group(hoa.library FILES ${hoa_library_src})
include_directories(${PROJECT_SOURCE_DIR}/hoa.library/Sources/)

add_pd_external(hoa.pi_project hoa.pi ${PROJECT_SOURCE_DIR}/sources/hoa.pi)
add_pd_external(hoa.dac_tilde_project hoa.dac~ ${PROJECT_SOURCE_DIR}/sources/hoa.dac_tilde)

file(GLOB hoa_externals
${PROJECT_SOURCE_DIR}/sources/hoa.pi
${PROJECT_SOURCE_DIR}/sources/hoa.dac_tilde)
#add_external(spam spam "${SPAM_SRC}")
