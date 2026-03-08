.PHONY: build build-xcode

prepare:
	mkdir -p build-xcode
	cmake -S . -B build-xcode -G Xcode

build:
	cmake --build build-xcode

clean:
	rm -rf build-xcode
