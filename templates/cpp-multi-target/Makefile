.PHONY: build

prepare:
	cmake --preset ninja-debug

build:
	cmake --build --preset ninja-debug

export_xcode_libs:
	cmake --preset xcode
	cmake --build --preset xcode

run: build
	sh ./run.sh

clean:
	rm -rf build


