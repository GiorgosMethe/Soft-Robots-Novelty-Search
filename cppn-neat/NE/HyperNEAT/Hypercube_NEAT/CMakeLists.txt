Project(NEAT)

IF(APPLE)
ADD_DEFINITIONS(
        -DTIXML_USE_STL
        -D__WXMAC__
        -DWXUSINGDLL
        -DNOPCH
        -D_USE_MATH_DEFINES
        -pthread
        -DBOOST_ALL_NO_LIB
        -DBOOST_PYTHON_STATIC_LIB
        -DNOMINMAX
        )
ELSE(APPLE)
ADD_DEFINITIONS(
        -DTIXML_USE_STL
        -D__WXGTK__
        -DWXUSINGDLL
        -DNOPCH
        -D_USE_MATH_DEFINES
        -pthread
        -DBOOST_ALL_NO_LIB
        -DBOOST_PYTHON_STATIC_LIB
        -DNOMINMAX
        )
ENDIF(APPLE)

IF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-fPIC)
ENDIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

SET(
        USE_GUI
        OFF
        CACHE
        BOOL
        "Should we add support for WxWidgets?"
)

ADD_DEFINITIONS(-DHCUBE_NOGUI)

SET(CMAKE_CXX_FLAGS "${CMAKE_WX_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D__WXDEBUG__ -DWXDEBUG=1")

INCLUDE_REGULAR_EXPRESSION( "^.*([Hh][Cc][Uu][Bb][Ee]|[Nn][Ee][Aa][Tt]|main).*$" )

SET(
        JGTL_INCLUDE
        "../../../JGTL/include"
        CACHE
        PATH
        "JGTL include directory"
)

SET(
        ZLIB_INCLUDE
        "../../../zlib"
        CACHE
        PATH
        "Zlib include directory"
)


find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

INCLUDE_DIRECTORIES(
        include
        ../NEAT/include
        src/utils
        ${TINYXMLDLL_INCLUDE}
        ${BOOST_ROOT}
        ${JGTL_INCLUDE}
        ${ZLIB_INCLUDE}
        ${CMAKE_SOURCE_DIR}/../../../VoxSim/VoxBotCreator
)

SET(
        VOXBOTCREATOR_LIB
        "${CMAKE_SOURCE_DIR}/../../../VoxSim/build/"
        CACHE
        PATH
        "VOXBOTCREATOR Library Directory"
)

find_library(libVoxBotCreator ${VOXBOTCREATOR_LIB})

find_file(libVoxBotCreator libVoxBotCreator.so ${VOXBOTCREATOR_LIB})
if(NOT libVoxBotCreator)
  message(SEND_ERROR "Cannot find library libVoxBotCreator.so")
else()
  message("Found libVoxBotCreator")
endif(NOT libVoxBotCreator)
LINK_DIRECTORIES(
        ${TINYXMLDLL_LIB}
        ${ZLIB_LIB}
        ${BOOST_STAGE}
        ${LIBRARY_OUTPUT_PATH}
        ${BOARD_LIB}
        ${VOXBOTCREATOR_LIB}
)

SOURCE_GROUP("Experiment Header Files" REGULAR_EXPRESSION include/Experiments/)
SOURCE_GROUP("Experiment Source Files" REGULAR_EXPRESSION src/Experiments/)

ADD_LIBRARY(
        Hypercube_NEAT_Base
        STATIC

        src/utils/Array3D.cpp

        src/HCUBE_Defines.cpp
        src/HCUBE_EvaluationPanel.cpp
        src/HCUBE_ExperimentPanel.cpp
        src/HCUBE_ExperimentRun.cpp
        src/HCUBE_EvaluationSet.cpp
        src/HCUBE_MainApp.cpp
        src/HCUBE_MainFrame.cpp
        src/HCUBE_NetworkPanel.cpp
        src/HCUBE_SubstratePanel.cpp
        src/HCUBE_GUITemplate.cpp
        src/HCUBE_ViewIndividualFrame.cpp
        src/HCUBE_UserEvaluationFrame.cpp
        src/HCUBE_NeuralNetworkPanel.cpp
        src/HCUBE_NeuralNetworkWeightGrid.cpp

        src/Experiments/HCUBE_XorExperiment.cpp
        src/Experiments/HCUBE_VoxbotsExperiment.cpp

        src/utils/Array3D.h

        include/HCUBE_Boost.h
        include/HCUBE_Defines.h
        include/HCUBE_EvaluationPanel.h
        include/HCUBE_EvaluationSet.h
        include/HCUBE_ExperimentPanel.h
        include/HCUBE_ExperimentRun.h
        include/HCUBE_MainApp.h
        include/HCUBE_MainFrame.h
        include/HCUBE_NetworkPanel.h
        include/HCUBE_STL.h
        include/HCUBE_SubstratePanel.h
        include/HCUBE_GUITemplate.h
        include/HCUBE_UserEvaluationFrame.h
        include/HCUBE_NeuralNetworkPanel.h
        include/HCUBE_NeuralNetworkWeightGrid.h
        include/HCUBE_ViewIndividualFrame.h

        include/Experiments/HCUBE_XorExperiment.h
        include/Experiments/HCUBE_Experiment.h
        include/Experiments/HCUBE_VoxbotsExperiment.h
        )

use_precompiled_header(
        Hypercube_NEAT_Base
        "${CMAKE_CURRENT_SOURCE_DIR}/include/HCUBE_Defines.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/HCUBE_Defines.cpp"
        )

ADD_DEPENDENCIES(Hypercube_NEAT_Base NEATLib)

SET_TARGET_PROPERTIES(Hypercube_NEAT_Base PROPERTIES DEBUG_POSTFIX _d)

ADD_EXECUTABLE(
        Hypercube_NEAT
        MACOSX_BUNDLE

        src/main.cpp
)

SET_TARGET_PROPERTIES(Hypercube_NEAT PROPERTIES DEBUG_POSTFIX _d)

SET(
        BOOST_LIB_EXT_NAME_DEBUG
        ""
        CACHE
        STRING
        "Boost debug extension"
        )
SET(
        BOOST_LIB_EXT_NAME_RELEASE
        ""
        CACHE
        STRING
        "Boost Release extension"
        )

SET(
        BOOST_LIB_PREFIX_NAME
        ""
        CACHE
        STRING
        "Boost prefix"
        )

TARGET_LINK_LIBRARIES(
        Hypercube_NEAT

        debug
        Hypercube_NEAT_Base_d

        optimized
        Hypercube_NEAT_Base

        debug
        NEATLib_d

        optimized
        NEATLib

        debug
        tinyxmlpluslib

        optimized
        tinyxmlpluslib

        debug
        zlib

        optimized
        zlib

        debug
        board

        optimized
        board

        debug
        ${BOOST_LIB_PREFIX_NAME}boost_thread

        optimized
        ${BOOST_LIB_PREFIX_NAME}boost_thread

        debug
        ${BOOST_LIB_PREFIX_NAME}boost_filesystem

        optimized
        ${BOOST_LIB_PREFIX_NAME}boost_filesystem

        debug
        ${BOOST_LIB_PREFIX_NAME}boost_system

        optimized
        ${BOOST_LIB_PREFIX_NAME}boost_system
)

TARGET_LINK_LIBRARIES(
        Hypercube_NEAT
        general
        pthread
        ${libVoxBotCreator}
        ${OpenCV_LIBS}
)

ADD_DEPENDENCIES(Hypercube_NEAT NEATLib Hypercube_NEAT_Base)
