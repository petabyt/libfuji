FLAGS := -DCMAKE_BUILD_TYPE=Release

b:
	cmake $(FLAGS) -G Ninja -B build
	cmake --build build

convert:
	cmake --build build && build/fudge --raw /home/daniel/Pictures/xt30-demo/DSCF2043.RAF  /home/daniel/Pictures/xt30-demo/output.jpg /home/daniel/Documents/fudge/lib/fp/fp1/Street.FP1

bw:
	cmake -DCMAKE_TOOLCHAIN_FILE=/home/$$USER/dotfiles/mingw.cmake -G Ninja -B build_win
	cmake --build build_win

bx:
	@echo "Toolchain file /home/$$USER/dotfiles/osx.cmake"
	cmake -DFUDGE_CREATE_MAC_APP=ON -DCMAKE_TOOLCHAIN_FILE=/home/$$USER/dotfiles/osx.cmake -G Ninja -B build_osx
	cmake --build build_osx

release:
	cmake -DFUDGE_CREATE_APPIMAGE=ON -G Ninja -B build
	cmake -DCMAKE_TOOLCHAIN_FILE=/home/$$USER/dotfiles/mingw.cmake -G Ninja -B build_win
	cmake -DFUDGE_CREATE_MAC_APP=ON -DCMAKE_TOOLCHAIN_FILE=/home/$$USER/dotfiles/osx.cmake -G Ninja -B build_osx

clean:
	rm -rf build_osx build_win build *.ini *.dat
