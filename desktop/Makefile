cmake_linux: build
	cmake --build build

build:
	cmake -G Ninja -B build

cmake_win: build_win
	cmake --build build_win

build_win:
	cmake -DCMAKE_TOOLCHAIN_FILE=cmake/mingw.cmake -G Ninja -B build_win
