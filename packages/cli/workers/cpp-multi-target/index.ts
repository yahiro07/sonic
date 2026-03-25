import {
  casingToCapital,
  casingToKebab,
  casingToSnake,
  TemplateWorker,
  workerHelper_copyProjectContentFiles,
  workerHelper_copyProjectContentFiles_withRenaming,
  workerHelper_getNewProjectFolderPath,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
} from "@/common";
import * as clackPrompts from "@clack/prompts";

type TemplateOptions = {
  includeFrameworkCode: boolean;
  includeDevHosts: boolean;
  includeEntryMakefile: boolean;
  renameFiles: boolean;
};

async function readTemplateOptions(): Promise<TemplateOptions | "cancelled"> {
  const includeFrameworkCode = await clackPrompts.confirm({
    message: `Include framework source code?`,
    initialValue: false,
  });
  if (clackPrompts.isCancel(includeFrameworkCode)) {
    return "cancelled";
  }
  const includeDevHosts = await clackPrompts.confirm({
    message: `Add VST3/CLAP development hosts?`,
    initialValue: true,
  });
  if (clackPrompts.isCancel(includeDevHosts)) {
    return "cancelled";
  }
  const includeEntryMakefile = await clackPrompts.confirm({
    message: `Add entry Makefile?`,
    initialValue: true,
  });
  if (clackPrompts.isCancel(includeEntryMakefile)) {
    return "cancelled";
  }
  const renameFiles = await clackPrompts.confirm({
    message: `Rename files?`,
    initialValue: true,
  });
  if (clackPrompts.isCancel(renameFiles)) {
    return "cancelled";
  }
  return {
    includeFrameworkCode,
    includeDevHosts,
    includeEntryMakefile,
    renameFiles,
  };
}

type TaskContext = {
  projectName: string;
  templateName: string;
  options: TemplateOptions;
  projectFolderPath: string;
};

function copyTemplateBaseFiles({ projectName, templateName }: TaskContext) {
  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "cmake/base-info.plist.in",
    "cmake/setup-sdks.cmake",
    "source",
    "wrapper",
    "www",
    ".clangd",
    ".gitignore",
    "CMakePresets.json",
  ]);

  workerHelper_copyProjectContentFiles_withRenaming(projectName, templateName, [
    { from: "CMakeLists_template.cmake", to: "CMakeLists.txt" },
  ]);
}

function copyFrameworkCodeIfNeeded({
  projectName,
  templateName,
  options,
}: TaskContext) {
  if (options.includeFrameworkCode) {
    workerHelper_copyProjectContentFiles(projectName, templateName, [
      "framework",
    ]);
  }
}

function renamePluginSourceFiles({
  projectName,
  projectFolderPath,
}: TaskContext) {
  const projectNameCapital = casingToCapital(projectName);
  const projectNameKebab = casingToKebab(projectName);
  const projectNameSnake = casingToSnake(projectName);

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: [
      "source/project1-synthesizer.cpp",
      "source/project1-synthesizer.h",
      "source/project1-factory.cpp",
    ],
    replacements: [
      {
        from: "Project1Synthesizer",
        to: `${projectNameCapital}Synthesizer`,
      },
      {
        from: "project1-synthesizer.h",
        to: `${projectNameKebab}-synthesizer.h`,
      },
      { from: `namespace project1`, to: `namespace ${projectNameSnake}` },
      { from: `project1::`, to: `${projectNameSnake}::` },
    ],
  });

  workerHelper_updateFileNamesWithPrefix(projectFolderPath, {
    filePaths: [
      "source/project1-synthesizer.cpp",
      "source/project1-synthesizer.h",
      "source/project1-factory.cpp",
    ],
    originalPrefix: "project1",
    newPrefix: projectNameKebab,
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["source/CMakeLists.txt"],
    replacements: [
      {
        from: "project1-synthesizer.cpp",
        to: `${projectNameKebab}-synthesizer.cpp`,
      },
      {
        from: "project1-factory.cpp",
        to: `${projectNameKebab}-factory.cpp`,
      },
    ],
  });
}

function patchVstWrapper({ projectName, projectFolderPath }: TaskContext) {
  const projectNameCapital = casingToCapital(projectName);
  const projectNameKebab = casingToKebab(projectName);

  const processorCID = crypto.randomUUID().toUpperCase();
  const controllerCID = crypto.randomUUID().toUpperCase();
  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/vst3/plugin-factory.cpp"],
    replacements: [
      { from: "E458F80E-DEED-40DB-AD59-2C739A7DA7A0", to: processorCID },
      { from: "6BAD2674-0204-4522-8971-58C6296A4552", to: controllerCID },
    ],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/vst3/version.h"],
    replacements: [{ from: "Project1", to: projectNameCapital }],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/vst3/CMakeLists.txt"],
    replacements: [{ from: "project1-vst3", to: `${projectNameKebab}-vst3` }],
  });
}

function patchClapWrapper({ projectName, projectFolderPath }: TaskContext) {
  const projectNameKebab = casingToKebab(projectName);

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/clap/CMakeLists.txt"],
    replacements: [{ from: "project1-clap", to: `${projectNameKebab}-clap` }],
  });
}

function patchCMakeLists({ options, projectFolderPath }: TaskContext) {
  const keepConditionalLine = (_text: string) => {};
  const removeConditionalLine = (text: string) => {
    workerHelper_replaceStrings(projectFolderPath, {
      filePaths: ["CMakeLists.txt"],
      replacements: [{ from: text + "\n", to: "" }],
    });
  };

  if (options.includeFrameworkCode) {
    keepConditionalLine(`add_subdirectory(framework/sonic)`);
    removeConditionalLine(
      `add_subdirectory("\${SONIC_ROOT_DIR}/templates/cpp-multi-target/framework/sonic"
                 "\${CMAKE_CURRENT_BINARY_DIR}/framework/sonic")`,
    );
  } else {
    removeConditionalLine(`add_subdirectory(framework/sonic)`);
    keepConditionalLine(
      `add_subdirectory("\${SONIC_ROOT_DIR}/templates/cpp-multi-target/framework/sonic"
                 "\${CMAKE_CURRENT_BINARY_DIR}/framework/sonic")`,
    );
  }

  if (!options.includeDevHosts) {
    removeConditionalLine(
      `add_subdirectory(\${SONIC_ROOT_DIR}/templates/_vst-dev-host/vst_dev_host
                 \${CMAKE_CURRENT_BINARY_DIR}/vst_dev_host)`,
    );
    removeConditionalLine(
      `add_subdirectory(\${SONIC_ROOT_DIR}/templates/_clap-dev-host/clap_dev_host
                 \${CMAKE_CURRENT_BINARY_DIR}/clap_dev_host)`,
    );
  }
}

function arrangeBuildWrapper({
  projectName,
  templateName,
  options,
  projectFolderPath,
}: TaskContext) {
  const hasMakefile = options.includeEntryMakefile;
  const hasRunScript = options.includeDevHosts;

  if (hasRunScript) {
    workerHelper_copyProjectContentFiles(projectName, templateName, ["run.sh"]);
  }

  if (hasMakefile) {
    workerHelper_copyProjectContentFiles(projectName, templateName, [
      "Makefile",
    ]);

    if (!hasRunScript) {
      workerHelper_replaceStrings(projectFolderPath, {
        filePaths: ["Makefile"],
        replacements: [
          {
            from: `run: build
	sh ./run.sh
`,
            to: "",
          },
        ],
      });
    }
  }
}

function scaffoldProject(
  projectName: string,
  templateName: string,
  options: TemplateOptions,
) {
  const taskContext: TaskContext = {
    projectName,
    templateName,
    options,
    projectFolderPath: workerHelper_getNewProjectFolderPath(projectName),
  };
  copyTemplateBaseFiles(taskContext);
  copyFrameworkCodeIfNeeded(taskContext);
  patchCMakeLists(taskContext);
  arrangeBuildWrapper(taskContext);
  if (options.renameFiles) {
    // applyTemplateCodeRenaming(taskContext);
    renamePluginSourceFiles(taskContext);
    patchVstWrapper(taskContext);
    patchClapWrapper(taskContext);
  }
  return true;
}

function createTemplateWorker(): TemplateWorker {
  return {
    async createProject(projectName, templateName) {
      console.log("cpp-multi-target worker");

      const templateOptions = await readTemplateOptions();
      if (templateOptions === "cancelled") {
        return "cancelled";
      }
      scaffoldProject(projectName, templateName, templateOptions);
      return true;
    },
  };
}
export default {
  name: "cpp-multi-target",
  description: "C++ multi target template for VST3/AUv3/CLAP",
  worker: createTemplateWorker(),
};
