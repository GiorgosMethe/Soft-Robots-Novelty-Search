Project(NEAT)

ADD_DEFINITIONS(
    -DBUILD_NEAT_DLL
    -DTIXML_USE_STL
    -DNOMINMAX
    -DBOOST_ALL_NO_LIB
)

INCLUDE_REGULAR_EXPRESSION( "^.*[Nn][Ee][Aa][Tt][_].*$" )

SET(
    TINYXMLDLL_INCLUDE
    "../../../tinyxmldll/include"
    CACHE
    PATH
    "TinyXMLDLL include directory"
)

SET(
    BOOST_ROOT
    "../../../../Libraries/boost-trunk"
    CACHE
    PATH
    "Boost Base Directory"
)

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

SET(
    BOARD_INCLUDE
    "../../../Board/include/"
    CACHE
    PATH
    "libboard include directory"
)

find_package( OpenCV )

INCLUDE_DIRECTORIES(
    include
    ${TINYXMLDLL_INCLUDE}
    ${BOOST_ROOT}
    ${JGTL_INCLUDE}
    ${ZLIB_INCLUDE}
    ${BOARD_INCLUDE}
    ${OpenCV_INCLUDE_DIRS}
)

SET(
    BOOST_STAGE
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../Libraries/boost-trunk/stage/lib"
    CACHE
    PATH
    "Boost Stage Directory"
)

SET(
    TINYXMLDLL_LIB
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../tinyxmldll/out/"
    CACHE
    PATH
    "TinyXMLDLL Library Directory"
)

SET(
    ZLIB_LIB
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../zlib/out/"
    CACHE
    PATH
    "Zlib Library Directory"
)

SET(
    BOARD_LIB
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../Board/out/"
    CACHE
    PATH
    "libboard library directory"
)

LINK_DIRECTORIES(
    ${TINYXMLDLL_LIB}
    ${ZLIB_LIB}
    ${BOOST_STAGE}
    ${BOARD_LIB}
)

SET(
    GPU_FILES

)

ADD_LIBRARY(
    NEATLib
    STATIC
    ${GPU_FILES}

    src/NEAT_Defines.cpp
    #src/NEAT_CoEvoExperiment.cpp
    src/NEAT_FastNetwork.cpp
    src/NEAT_FastLayeredNetwork.cpp
    src/NEAT_FastBiasNetwork.cpp
    src/NEAT_FractalNetwork.cpp
    src/NEAT_GeneticGene.cpp
    src/NEAT_GeneticGeneration.cpp
    src/NEAT_CoEvoGeneticGeneration.cpp
    src/NEAT_GeneticIndividual.cpp
    src/NEAT_GeneticLinkGene.cpp
    src/NEAT_GeneticNodeGene.cpp
    src/NEAT_GeneticPopulation.cpp
    src/NEAT_GeneticSpecies.cpp
    src/NEAT_Globals.cpp
    src/NEAT_ModularNetwork.cpp
    src/NEAT_VectorNetwork.cpp
    src/NEAT_Network.cpp
    src/NEAT_NetworkLink.cpp
    src/NEAT_NetworkNode.cpp
    src/NEAT_Random.cpp
    src/NEAT_LayeredSubstrate.cpp
    src/NEAT_GeneticIndividualBehavior.cpp

    include/NEAT_GeneticIndividualBehavior.h
    include/NEAT_CoEvoExperiment.h
    include/NEAT_FastNetwork.h
    include/NEAT_FastLayeredNetwork.h
    include/NEAT_FastBiasNetwork.h
    include/NEAT_FractalNetwork.h
    include/NEAT_GeneticGene.h
    include/NEAT_GeneticGeneration.h
    include/NEAT_CoEvoGeneticGeneration.h
    include/NEAT_GeneticIndividual.h
    include/NEAT_GeneticLinkGene.h
    include/NEAT_GeneticNodeGene.h
    include/NEAT_GeneticPopulation.h
    include/NEAT_GeneticSpecies.h
    include/NEAT_Globals.h
    include/NEAT.h
    include/NEAT_Defines.h
    include/NEAT_ModularNetwork.h
    include/NEAT_Network.h
    include/NEAT_VectorNetwork.h
    include/NEAT_NetworkLink.h
    include/NEAT_NetworkNode.h
    include/NEAT_Random.h
    include/NEAT_STL.h
    include/NEAT_LayeredSubstrate.h
)

use_precompiled_header(
    NEATLib
    "${CMAKE_CURRENT_SOURCE_DIR}/include/NEAT_Defines.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/NEAT_Defines.cpp"
)

SET_TARGET_PROPERTIES(NEATLib PROPERTIES DEBUG_POSTFIX _d)

