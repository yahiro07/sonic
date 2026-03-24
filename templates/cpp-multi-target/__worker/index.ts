import {
  TemplateWorker,
  workerHelper_copyProjectContentFiles,
  workerHelper_copyProjectContentFiles_withRenaming,
  workerHelper_getNewProjectFolderPath,
  workerHelper_replaceStrings,
} from "@/common";
import * as clackPrompts from "@clack/prompts";

type TemplateOptions = {
  includeFrameworkCode: boolean;
  includeDevHosts: boolean;
  includeEntryMakefile: boolean;
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
  return {
    includeFrameworkCode,
    includeDevHosts,
    includeEntryMakefile,
  };
}

type TaskContext = {
  projectName: string;
  templateName: string;
  options: TemplateOptions;
  newFolderPath: string;
};

function copyTemplateBaseFiles({ projectName, templateName }: TaskContext) {
  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "cmake/base-info.plist.in",
    "cmake/setup-sdks.cmake",
    "plugin",
    "variants",
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

// function applyTemplateCodeRenaming(projectName: string) {
//   const newFolderPath = workerHelper_getNewProjectFolderPath(projectName);

//   const projectNameSnake = casingToSnake(projectName);
//   const projectNameCapital = casingToCapital(projectName);

//   workerHelper_replaceStrings(newFolderPath, {
//     filePaths: [
//       "source/project1_synthesizer.cpp",
//       "source/project1_synthesizer.h",
//     ],
//     replacements: [
//       {
//         from: "Project1Synthesizer",
//         to: `${projectNameCapital}Synthesizer`,
//       },
//       {
//         from: "project1_synthesizer.h",
//         to: `${projectNameSnake}_synthesizer.h`,
//       },
//     ],
//   });

//   workerHelper_updateFileNamesWithPrefix(newFolderPath, {
//     filePaths: [
//       "source/project1_synthesizer.cpp",
//       "source/project1_synthesizer.h",
//     ],
//     originalPrefix: "project1",
//     newPrefix: projectNameSnake,
//   });

//   workerHelper_replaceStrings(newFolderPath, {
//     filePaths: ["source/version.h"],
//     replacements: [{ from: "Project1", to: projectNameCapital }],
//   });

//   const processorCID = crypto.randomUUID().toUpperCase();
//   const controllerCID = crypto.randomUUID().toUpperCase();

//   workerHelper_replaceStrings(newFolderPath, {
//     filePaths: ["source/plugin_factory.cpp"],
//     replacements: [
//       {
//         from: "project1_synthesizer.h",
//         to: `${projectNameSnake}_synthesizer.h`,
//       },
//       { from: "MyPlugin", to: projectNameCapital },
//       { from: "E458F80E-DEED-40DB-AD59-2C739A7DA7A0", to: processorCID },
//       { from: "6BAD2674-0204-4522-8971-58C6296A4552", to: controllerCID },
//     ],
//   });

//   workerHelper_replaceStrings(newFolderPath, {
//     filePaths: ["CMakeLists.txt"],
//     replacements: [
//       {
//         from: "project1_synthesizer.cpp",
//         to: `${projectNameSnake}_synthesizer.cpp`,
//       },
//       { from: "Project1", to: projectNameCapital },
//     ],
//   });

//   if (workerHelper_checkFileExists(newFolderPath, "Makefile")) {
//     workerHelper_replaceStrings(newFolderPath, {
//       filePaths: ["Makefile"],
//       replacements: [
//         {
//           from: "Project1.vst3",
//           to: `${casingToCapital(projectName)}.vst3`,
//         },
//       ],
//     });
//   }
// }

function patchCMakeLists({ options, newFolderPath }: TaskContext) {
  const keepConditionalLine = (_text: string) => {};
  const removeConditionalLine = (text: string) => {
    workerHelper_replaceStrings(newFolderPath, {
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
  newFolderPath,
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
      workerHelper_replaceStrings(newFolderPath, {
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
    newFolderPath: workerHelper_getNewProjectFolderPath(projectName),
  };
  copyTemplateBaseFiles(taskContext);
  copyFrameworkCodeIfNeeded(taskContext);
  patchCMakeLists(taskContext);
  arrangeBuildWrapper(taskContext);
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
