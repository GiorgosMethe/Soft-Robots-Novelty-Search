cd tinyxmldll
if [ -d build ]; then
	rm -rf build
fi
mkdir build
cd build
cmake ..
make -j 8

cd ../../

cd Board
if [ -d build ]; then
	rm -rf build
fi
mkdir build
cd build
cmake ..
make -j 8

cd ../../

cd zlib
if [ -d build ]; then
	rm -rf build
fi
mkdir build
cd build
cmake ..
make -j 8

cd ../../

cd JGTL
if [ -d build ]; then
	rm -rf build
fi
mkdir build
cd build
cmake ..
make -j 8

cd ../../

cd NE/HyperNEAT
if [ -d build ]; then
	rm -rf build
fi
mkdir build
cd build
cmake ..
make -j 8




