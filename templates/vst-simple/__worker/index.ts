import {
  TemplateWorker,
  casingToCapital,
  casingToSnake,
  workerHelper_copyProjectContentFiles,
  workerHelper_copyProjectContentFiles_withRenaming,
  workerHelper_getNewProjectFolderPath,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
} from "@/common";
import * as clackPrompts from "@clack/prompts";

type TemplateOptions = {
  useExtensibleBase: boolean;
  includeVstDevHost: boolean;
  buildWrapper: "none" | "cmakePresets" | "makefile";
};

async function readTemplateOptions(): Promise<TemplateOptions | "cancelled"> {
  const includeVstDevHost = await clackPrompts.confirm({
    message: `Include VST development host?`,
    initialValue: true,
  });
  if (clackPrompts.isCancel(includeVstDevHost)) {
    return "cancelled";
  }
  const buildWrapper = await clackPrompts.select({
    message: "Add build wrapper",
    initialValue: "none",
    options: [
      { value: "none", label: "None" },
      { value: "cmakePresets", label: "CMakePresets.json" },
      { value: "makefile", label: "Makefile" },
    ] satisfies { value: TemplateOptions["buildWrapper"]; label: string }[],
  });
  if (clackPrompts.isCancel(buildWrapper)) {
    return "cancelled";
  }
  const useExtensibleBase = await clackPrompts.confirm({
    message: `Use extensible code structure? (for advanced users)`,
    initialValue: false,
  });
  if (clackPrompts.isCancel(useExtensibleBase)) {
    return "cancelled";
  }
  return {
    useExtensibleBase,
    includeVstDevHost,
    buildWrapper: buildWrapper as TemplateOptions["buildWrapper"],
  };
}

function copyTemplateFiles(
  projectName: string,
  templateName: string,
  options: TemplateOptions,
) {
  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "frontend",
    // "sonic",  //skip copying, it's downloaded from github with FetchContent
    "resource",
    "source",
    "CMakeLists.txt",
    "README.md",
  ]);

  workerHelper_copyProjectContentFiles_withRenaming(projectName, templateName, [
    { from: "Makefile_template", to: "Makefile" },
    { from: ".gitignore_template", to: ".gitignore" },
  ]);

  //TODO: patch makefile, change build system based on OS

  const newFolderPath = workerHelper_getNewProjectFolderPath(projectName);

  const projectNameSnake = casingToSnake(projectName);
  const projectNameCapital = casingToCapital(projectName);

  workerHelper_replaceStrings(newFolderPath, {
    filePaths: [
      "source/project1_synthesizer.cpp",
      "source/project1_synthesizer.h",
    ],
    replacements: [
      {
        from: "Project1Synthesizer",
        to: `${projectNameCapital}Synthesizer`,
      },
      {
        from: "project1_synthesizer.h",
        to: `${projectNameSnake}_synthesizer.h`,
      },
    ],
  });

  workerHelper_updateFileNamesWithPrefix(newFolderPath, {
    filePaths: [
      "source/project1_synthesizer.cpp",
      "source/project1_synthesizer.h",
    ],
    originalPrefix: "project1",
    newPrefix: projectNameSnake,
  });

  workerHelper_replaceStrings(newFolderPath, {
    filePaths: ["source/version.h"],
    replacements: [{ from: "Project1", to: projectNameCapital }],
  });

  workerHelper_replaceStrings(newFolderPath, {
    filePaths: ["CMakeLists.txt"],
    replacements: [
      {
        from: "project1_synthesizer.cpp",
        to: `${projectNameSnake}_synthesizer.cpp`,
      },
      {
        from: "Project1",
        to: projectNameCapital,
      },
    ],
  });

  const processorCID = crypto.randomUUID().toUpperCase();
  const controllerCID = crypto.randomUUID().toUpperCase();

  workerHelper_replaceStrings(newFolderPath, {
    filePaths: ["source/plugin_factory.cpp"],
    replacements: [
      {
        from: "project1_synthesizer.h",
        to: `${projectNameSnake}_synthesizer.h`,
      },
      { from: "MyPlugin", to: projectNameCapital },
      { from: "E458F80E-DEED-40DB-AD59-2C739A7DA7A0", to: processorCID },
      { from: "6BAD2674-0204-4522-8971-58C6296A4552", to: controllerCID },
    ],
  });
  return true;
}

function createTemplateWorker(): TemplateWorker {
  return {
    async createProject(projectName, templateName) {
      console.log("vst-simple worker");

      const templateOptions = await readTemplateOptions();
      if (templateOptions === "cancelled") {
        return "cancelled";
      }
      copyTemplateFiles(projectName, templateName, templateOptions);
    },
  };
}
export default {
  name: "vst-simple",
  description: "VST Simple Template",
  worker: createTemplateWorker(),
};
