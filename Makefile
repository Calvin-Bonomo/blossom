.PHONY: clean

release:
	mkdir -p build/release
	cmake -B build/release -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	cmake --build build/release

debug:
	mkdir -p build/debug
	cmake -B build/debug -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja
	cmake --build build/debug

clean:
	rm -rf ./build
