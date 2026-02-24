CLI_HOST = /Users/ore/Documents/projects/yahiro/synth2511/vstdev/VstDevHost/build/bin/Debug/VstDevHost.app/Contents/MacOS/VstDevHost
GUI_HOST = ~/Documents/projects/pre/vst/work1-juce/lib/JUCE/extras/AudioPluginHost/Builds/MacOSX/build/Debug/AudioPluginHost.app

VST3_OUT = build/VST3/Debug/Project1.vst3

.PHONY: build

default: build

prepare:
	cmake -B build -G Ninja -DSMTG_RUN_VST_VALIDATOR=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  -DCMAKE_BUILD_TYPE=Debug .
	ln -sf build/compile_commands.json .

build:
	cmake --build build

run: build
# 	$(CLI_HOST) $(VST3_OUT)
	open $(GUI_HOST)

clean:
	# Keep build/_deps to avoid re-downloading dependencies (vst3sdk, saucer, etc.)
	@if [ -d build ]; then \
		find build -mindepth 1 -maxdepth 1 ! -name _deps -exec rm -rf {} +; \
	fi
	rm -rf .cache


full: clean prepare run
