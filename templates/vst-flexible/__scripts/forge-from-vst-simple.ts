import path from "path";
import fs from "fs";

//vst-flexible is forked from vst-simple, so we can copy most of the files and patch some of them

function copyEntry(
  sourcePath: string,
  destinationPath: string,
  excludeDirs?: string[],
) {
  console.log(`Copying ${sourcePath} to ${destinationPath}`);
  if (fs.existsSync(sourcePath) && fs.lstatSync(sourcePath).isDirectory()) {
    fs.rmSync(destinationPath, { recursive: true, force: true });
    fs.cpSync(sourcePath, destinationPath, {
      recursive: true,
      filter: excludeDirs
        ? (srcPath) => {
            const name = path.basename(srcPath);
            if (excludeDirs.includes(name)) {
              return false;
            }
            return true;
          }
        : undefined,
    });
  } else {
    fs.copyFileSync(sourcePath, destinationPath);
  }
}

function moveEntry(sourcePath: string, destinationPath: string) {
  console.log(`Moving ${sourcePath} to ${destinationPath}`);
  fs.rmSync(destinationPath, { recursive: true, force: true });
  fs.renameSync(sourcePath, destinationPath);
}

function patchFileContent(
  filePath: string,
  spec: { from: string; to: string },
) {
  const { from, to } = spec;
  //replace file content text, raise if no matching
  console.log(`Patching ${filePath}: ${from} -> ${to}`);
  const content = fs.readFileSync(filePath, "utf-8");
  if (!content.includes(from)) {
    throw new Error(`Content not found in ${filePath}: ${from}`);
  }
  const newContent = content.replace(from, to);
  fs.writeFileSync(filePath, newContent, "utf-8");
}

function patchFileContentMultiple(
  filePath: string,
  replacements: { from: string; to: string }[],
) {
  for (const spec of replacements) {
    patchFileContent(filePath, spec);
  }
}

function run() {
  console.log("forging template vst-flexible from vst-simple...");

  const baseFolderPath = path.resolve("../");
  console.log(baseFolderPath);

  const sourceTemplateFolderPath = path.join(baseFolderPath, "../vst-simple");
  console.log(sourceTemplateFolderPath);

  for (const entry of [
    "__worker",
    "frontend",
    "sonic",
    "resource",
    "source",
    ".gitignore",
    "CMakeLists.txt",
    "Makefile",
    "Makefile_exported",
    "README.md",
  ]) {
    const sourcePath = path.join(sourceTemplateFolderPath, entry);
    const destinationPath = path.join(baseFolderPath, entry);
    copyEntry(sourcePath, destinationPath, ["node_modules", ".vite"]);
  }

  function relativeFromBase(filePath: string) {
    return path.join(baseFolderPath, filePath);
  }

  //disable FetchContent for sonic, use local implementation
  patchFileContent(relativeFromBase("CMakeLists.txt"), {
    from: `#----------------
#download sonic framework

if(USE_LOCAL_SONIC)
    add_subdirectory("sonic")
else()
    FetchContent_Declare(
        sonic
        GIT_REPOSITORY https://github.com/yahiro07/sonic.git
        GIT_TAG main
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR  templates/vst-simple/sonic
    )
    FetchContent_MakeAvailable(sonic)
endif()
#----------------`,
    to: `add_subdirectory("sonic")`,
  });

  //move sonic/vst_basis to source/vst_basis
  moveEntry(
    relativeFromBase("sonic/vst_basis"),
    relativeFromBase("source/vst_basis"),
  );
  patchFileContent(relativeFromBase("sonic/CMakeLists.txt"), {
    from: `  #--
  vst_basis/plugin_processor.cpp
  vst_basis/plugin_controller.cpp`,
    to: "",
  });

  //move version.h and plugin_factory.cpp
  moveEntry(
    relativeFromBase("source/version.h"),
    relativeFromBase("source/vst_basis/version.h"),
  );
  moveEntry(
    relativeFromBase("source/plugin_factory.cpp"),
    relativeFromBase("source/vst_basis/plugin_factory.cpp"),
  );

  //patch plugin_factory.cpp
  patchFileContentMultiple(
    relativeFromBase("source/vst_basis/plugin_factory.cpp"),
    [
      {
        from: `#include "sonic/vst_basis/plugin_controller.h"`,
        to: `#include "./plugin_controller.h"`,
      },
      {
        from: `#include "sonic/vst_basis/plugin_processor.h"`,
        to: `#include "./plugin_processor.h"`,
      },
      {
        from: `#include "./project1_synthesizer.h"`,
        to: `#include "../project1_synthesizer.h"`,
      },
    ],
  );

  //patch CMakeLists.txt
  patchFileContentMultiple(relativeFromBase("CMakeLists.txt"), [
    {
      from: `# source/vst_basis/plugin_processor.cpp`,
      to: `source/vst_basis/plugin_processor.cpp`,
    },
    {
      from: `# source/vst_basis/plugin_controller.cpp`,
      to: `source/vst_basis/plugin_controller.cpp`,
    },
    {
      from: `source/version.h`,
      to: `source/vst_basis/version.h`,
    },
    {
      from: `source/plugin_factory.cpp`,
      to: `source/vst_basis/plugin_factory.cpp`,
    },
  ]);

  //patch readme
  patchFileContent(relativeFromBase("README.md"), {
    from: "sonic template project (vst-simple)",
    to: "sonic template project (vst-flexible)",
  });

  //patch worker
  patchFileContentMultiple(relativeFromBase("__worker/index.ts"), [
    {
      from: `// "sonic",  //skip copying, it's downloaded from github with FetchContent`,
      to: `"sonic",`,
    },
    {
      from: `console.log("vst-simple worker");`,
      to: `console.log("vst-flexible worker");`,
    },
    {
      from: `  name: "vst-simple",
  description: "VST Simple Template",`,
      to: `  name: "vst-flexible",
  description: "VST Flexible Template",`,
    },
  ]);
}
run();
