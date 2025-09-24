.PHONY: clean

release:
	mkdir -p ./build/release
	cmake -B ./build/release -S . -DCMAKE_BUILD_TYPE=Release
	cmake --build build/release

debug:
	mkdir -p ./build/release
	cmake -B ./build/debug -S . -DCMAKE_BUILD_TYPE=Debug
	cmake --build build/debug

clean:
	rm -rf ./build
