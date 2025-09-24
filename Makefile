release:
	mkdir -p build/release
	cmake -B build/release -S . -DCMAKE_BUILD_TYPE=Release -G Ninja
	cmake --build build/release

debug:
	mkdir -p build/release
	cmake -B build/debug -S . -DCMAKE_BUILD_TYPE=Debug -G Ninja
	cmake --build build/debug
