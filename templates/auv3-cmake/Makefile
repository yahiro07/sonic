.PHONY: build build

prepare:
	mkdir -p build
	cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1

build:
	cmake --build build

clean:
	rm -rf build
