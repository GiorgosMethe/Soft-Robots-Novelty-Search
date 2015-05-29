project( GaDIrectEncoding )
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Release )
add_definitions ( -Wall )
find_package ( Qt4 REQUIRED )
include (${QT_USE_FILE})

include_directories (
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${QT_QTCORE_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/galib247/ga/include/
        ${CMAKE_SOURCE_DIR}/../VoxSim/VoxBotCreator
        )

set ( GaDIrectEncoding_HDRS
                PetriDish/PetriDish.h
        )

set ( GaDIrectEncoding_SRCS
        GaDirectEncoding.cpp
        PetriDish/PetriDish.cpp
)

find_library(libVoxBotCreator "../VoxSim/build")

find_file(libVoxBotCreator libVoxBotCreator.so "../VoxSim/build")
if(NOT libVoxBotCreator)
  message(SEND_ERROR "Cannot find library libVoxBotCreator.so")
else()
  message("Found libVoxBotCreator")
endif(NOT libVoxBotCreator)

find_library(libga "galib247/ga")

find_file(libga libga.a "galib247/ga")
if(NOT libga)
  message(SEND_ERROR "Cannot find library libga.a")
else()
  message("Found libga")
endif(NOT libga)

find_package( Threads )

add_executable ( GaDIrectEncoding ${GaDIrectEncoding_SRCS} ${RSCS} ${libVoxBotCreator} ${libga})
target_link_libraries ( GaDIrectEncoding  ${QT_QTCORE_LIBRARY} ${libVoxBotCreator} ${libga} ${CMAKE_THREAD_LIBS_INIT})
