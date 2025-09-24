release:
	cmake -B build/release -S . -DCMAKE_BUILD_TYPE=Release -G Ninja
	cmake --build build/release

debug:
	cmake -B build/debug -S . -DCMAKE_BUILD_TYPE=Debug -G Ninja
	cmake --build build/debug
