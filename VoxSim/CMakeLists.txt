project( VoxSim )
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Release )
add_definitions ( -Wall -DPREC_MAX -DQT_XML_LIB)
find_package ( Qt4 REQUIRED QtXml )
find_package(Qt COMPONENTS QtXml REQUIRED)

include ( ${QT_USE_FILE} )

FIND_PACKAGE(Boost COMPONENTS filesystem system)

include_directories (
	${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        ${QT_QTCORE_INCLUDE_DIR})

include_directories ( ${Boost_INCLUDE_DIRS} )
link_directories ( ${Boost_LIBRARY_DIRS} )

set ( VoxSim_HDRS
	Voxelyze/Utils/zlib.h
	Voxelyze/Utils/zconf.h
	Voxelyze/Utils/XML_Rip.h
	Voxelyze/Utils/Vec3D.h
	Voxelyze/Utils/tinyxml.h
	Voxelyze/Utils/Mesh.h
	Voxelyze/Utils/MarchCube.h
        Voxelyze/Utils/GL_Utils.h
	Voxelyze/Utils/Array3D.h
	Voxelyze/VXS_Voxel.h
	Voxelyze/VXS_BondInternal.h
	Voxelyze/VXS_BondCollision.h
	Voxelyze/VXS_Bond.h
	Voxelyze/VX_Voxel.h
	Voxelyze/VX_SimGA.h
	Voxelyze/VX_Sim.h
	Voxelyze/VX_Object.h
	Voxelyze/VX_MeshUtil.h
	Voxelyze/VX_FRegion.h
	Voxelyze/VX_FEA.h
	Voxelyze/VX_Environment.h
	Voxelyze/VX_Enums.h
	Voxelyze/VX_Bond.h
	Voxelyze/VX_Benchmark.h
	)

set ( VoxSim_SRCS
        Main.cpp
	Voxelyze/Utils/XML_Rip.cpp
	Voxelyze/Utils/tinyxmlparser.cpp
	Voxelyze/Utils/tinyxmlerror.cpp
	Voxelyze/Utils/tinyxml.cpp
	Voxelyze/Utils/Mesh.cpp
	Voxelyze/Utils/MarchCube.cpp
	Voxelyze/Utils/GL_Utils.cpp
	Voxelyze/Utils/Array3D.cpp
	Voxelyze/VXS_Voxel.cpp
	Voxelyze/VXS_BondInternal.cpp
	Voxelyze/VXS_BondCollision.cpp
	Voxelyze/VXS_Bond.cpp
	Voxelyze/VX_Voxel.cpp
	Voxelyze/VX_SimGA.cpp
	Voxelyze/VX_Sim.cpp
	Voxelyze/VX_Object.cpp
	Voxelyze/VX_MeshUtil.cpp
	Voxelyze/VX_FRegion.cpp
	Voxelyze/VX_FEA.cpp
	Voxelyze/VX_Environment.cpp
	Voxelyze/VX_Bond.cpp
	Voxelyze/VX_Benchmark.cpp
	)

set ( VoxBotCreator_HDRS
        VoxBotCreator/voxbotcreator_global.h
	VoxBotCreator/VoxBotCreator.h
	Voxelyze/Utils/zlib.h
	Voxelyze/Utils/zconf.h
	Voxelyze/Utils/XML_Rip.h
	Voxelyze/Utils/Vec3D.h
	Voxelyze/Utils/tinyxml.h
	Voxelyze/Utils/Mesh.h
	Voxelyze/Utils/MarchCube.h
        Voxelyze/Utils/GL_Utils.h
	Voxelyze/Utils/Array3D.h
	Voxelyze/VXS_Voxel.h
	Voxelyze/VXS_BondInternal.h
	Voxelyze/VXS_BondCollision.h
	Voxelyze/VXS_Bond.h
	Voxelyze/VX_Voxel.h
	Voxelyze/VX_SimGA.h
	Voxelyze/VX_Sim.h
	Voxelyze/VX_Object.h
	Voxelyze/VX_MeshUtil.h
	Voxelyze/VX_FRegion.h
	Voxelyze/VX_FEA.h
	Voxelyze/VX_Environment.h
	Voxelyze/VX_Enums.h
	Voxelyze/VX_Bond.h
	Voxelyze/VX_Benchmark.h
)

set ( VoxBotCreator_SRCS
	VoxBotCreator/VoxBotCreator.cpp
	Voxelyze/Utils/XML_Rip.cpp
	Voxelyze/Utils/tinyxmlparser.cpp
	Voxelyze/Utils/tinyxmlerror.cpp
	Voxelyze/Utils/tinyxml.cpp
	Voxelyze/Utils/Mesh.cpp
	Voxelyze/Utils/MarchCube.cpp
	Voxelyze/Utils/GL_Utils.cpp
	Voxelyze/Utils/Array3D.cpp
	Voxelyze/VXS_Voxel.cpp
	Voxelyze/VXS_BondInternal.cpp
	Voxelyze/VXS_BondCollision.cpp
	Voxelyze/VXS_Bond.cpp
	Voxelyze/VX_Voxel.cpp
	Voxelyze/VX_SimGA.cpp
	Voxelyze/VX_Sim.cpp
	Voxelyze/VX_Object.cpp
	Voxelyze/VX_MeshUtil.cpp
	Voxelyze/VX_FRegion.cpp
	Voxelyze/VX_FEA.cpp
	Voxelyze/VX_Environment.cpp
	Voxelyze/VX_Bond.cpp
	Voxelyze/VX_Benchmark.cpp
)

ADD_CUSTOM_COMMAND(
   OUTPUT help.txt
   COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/help.txt ${CMAKE_CURRENT_BINARY_DIR}/help.txt
   )

ADD_CUSTOM_TARGET(help ALL DEPENDS help.txt)

add_executable ( VoxSim ${VoxSim_SRCS} ${help})
target_link_libraries ( VoxSim  ${QT_QTCORE_LIBRARY} ${QT_QTXML_LIBRARY} ${Boost_LIBRARIES})

add_library ( VoxBotCreator SHARED ${VoxBotCreator_SRCS} ${UIS} ${RSCS} ${TRS} ${MOCS} )
target_link_libraries ( VoxBotCreator  ${QT_QTCORE_LIBRARY} ${QT_QTXML_LIBRARY} ${Boost_LIBRARIES})
