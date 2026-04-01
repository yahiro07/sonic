## Template Development

to develop or maintain templates, the development steps would be like below:

1. Debug template project in place
2. Write or adjust worker script
3. Instantiate and debug templated project

### Related file or folders

#### `sonic/templates/cpp-multi-target`

#### `sonic/templates/cpp-multi-target/CMakeLists.txt`

## Debug template project in place

template source project (`sonic/templates/<template-name>`) is itself a working project that can be built and run.

```bash
  cd sonic/templates/cpp-multi-target
	cmake --preset ninja-debug
	cmake --build --preset ninja-debug
  ./build/ninja-debug/bin/Debug/VstDevHost ./build/ninja-debug/lib/Debug/project1-vst3.vst3
```

## Write or adjust worker script

the worker script for the template is located in `sonic/cli/src/workers/<template-name>/index.ts`.

It reads prompts and copy or patch required files according to the configuration.
`sonic/cli/src/workers/template-entries.ts` is a list of templates.

## Instantiate and debug templated project

```bash
  cd sonic/cli
  CLI_DEBUG_TEMPLATE_WORKERS=1 npx tsx src/index.ts
  cd CppMultiTarget1
  cmake --preset ninja-debug
	cmake --build --preset ninja-debug
  ./build/ninja-debug/bin/Debug/VstDevHost ./build/ninja-debug/lib/Debug/project1-vst3.vst3
```

Using tsx, CLI can be executed directly from source without build.
Follow the wizard to create a project, build it, and check it.

The development flag `CLI_DEBUG_TEMPLATE_WORKERS=1` tells CLI to refer `sonic/templates` directly instead of `sonic/cli/templates`. In production, CLI uses `sonic/cli/templates`.
