b:
	cmake -DFUDGE_CREATE_APPIMAGE=ON -G Ninja -B build
	cmake --build build

bw:
	cmake -DCMAKE_TOOLCHAIN_FILE=/home/$$USER/dotfiles/mingw.cmake -G Ninja -B build_win
	cmake --build build_win

bx:
	@echo "Toolchain file /home/$$USER/dotfiles/osx.cmake"
	cmake -DCMAKE_TOOLCHAIN_FILE=/home/$$USER/dotfiles/osx.cmake -G Ninja -B build_osx
	cmake --build build_osx

clean:
	rm -rf build_osx build_win build *.ini *.dat
