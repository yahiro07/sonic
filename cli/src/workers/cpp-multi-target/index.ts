import {
  casingToCapital,
  casingToKebab,
  casingToSnake,
  generateRandomString,
  TemplateWorker,
  workerHelper_copyProjectContentFiles,
  workerHelper_copyProjectContentFiles_withRenaming,
  workerHelper_getNewProjectFolderPath,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
} from "@/src/common";
import * as clackPrompts from "@clack/prompts";

type TemplateOptions = {
  platforms: ("vst3" | "clap" | "auv3")[];
  includeFrameworkCode: boolean;
  includeDevHosts: boolean;
  includeEntryMakefile: boolean;
  auManufacturer: string;
  auSubtype: string;
};

async function readTemplateOptions(): Promise<TemplateOptions | "cancelled"> {
  const tmpPlatforms = await clackPrompts.multiselect({
    message: "Pick target platforms",
    options: [
      { value: "vst3", label: "VST3" },
      { value: "clap", label: "CLAP" },
      { value: "auv3", label: "AUv3" },
    ],
    initialValues: ["vst3", "clap", "auv3"],
  });
  if (clackPrompts.isCancel(tmpPlatforms)) {
    return "cancelled";
  }
  const platforms = tmpPlatforms as TemplateOptions["platforms"];

  const includeFrameworkCode = await clackPrompts.confirm({
    message: `Include framework source code?`,
    initialValue: false,
  });
  if (clackPrompts.isCancel(includeFrameworkCode)) {
    return "cancelled";
  }

  let includeDevHosts: boolean | symbol = false;
  if (platforms.includes("vst3") || platforms.includes("clap")) {
    includeDevHosts = await clackPrompts.confirm({
      message: `Add VST3/CLAP development hosts?`,
      initialValue: true,
    });
    if (clackPrompts.isCancel(includeDevHosts)) {
      return "cancelled";
    }
  }

  const includeEntryMakefile = await clackPrompts.confirm({
    message: `Add entry Makefile?`,
    initialValue: true,
  });
  if (clackPrompts.isCancel(includeEntryMakefile)) {
    return "cancelled";
  }

  let auManufacturer: string | symbol = "";
  let auSubtype: string | symbol = "";
  if (platforms.includes("auv3")) {
    const auManufacturerDefault = "Myco";
    auManufacturer = await clackPrompts.text({
      message: `AUv3 Manufacturer Code`,
      placeholder: auManufacturerDefault,
    });
    if (clackPrompts.isCancel(auManufacturer)) {
      return "cancelled";
    }
    if (auManufacturer == "") {
      auManufacturer = auManufacturerDefault;
    }
    const auSubtypeDefault = generateRandomString("alphaNumeric", 4);
    auSubtype = await clackPrompts.text({
      message: `AUv3 Subtype`,
      placeholder: auSubtypeDefault,
    });
    if (clackPrompts.isCancel(auSubtype)) {
      return "cancelled";
    }
    if (auSubtype == "") {
      auSubtype = auSubtypeDefault;
    }
  }

  return {
    includeFrameworkCode,
    includeDevHosts,
    includeEntryMakefile,
    auManufacturer,
    auSubtype,
    platforms,
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
    "www",
    ".clangd",
    "CMakePresets.json",
  ]);

  workerHelper_copyProjectContentFiles_withRenaming(projectName, templateName, [
    { from: ".gitignore_template", to: ".gitignore" },
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

function addVstWrapper({
  projectName,
  templateName,
  projectFolderPath,
}: TaskContext) {
  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "wrapper/vst3",
  ]);

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

function addClapWrapper({
  projectName,
  templateName,
  projectFolderPath,
}: TaskContext) {
  const projectNameKebab = casingToKebab(projectName);

  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "wrapper/clap",
  ]);

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/clap/CMakeLists.txt"],
    replacements: [{ from: "project1-clap", to: `${projectNameKebab}-clap` }],
  });
}

function addAuv3XcodeProject({
  projectName,
  templateName,
  options,
  projectFolderPath,
}: TaskContext) {
  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "wrapper/auv3-xcode-project/Project1",
    "wrapper/auv3-xcode-project/Project1.xcodeproj/project.pbxproj",
    "wrapper/auv3-xcode-project/Project1Extension",
  ]);

  const projectNameCapital = casingToCapital(projectName);
  const extensionNameCapital = `${projectNameCapital}Extension`;

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/auv3-xcode-project/Project1/Project1App.swift"],
    replacements: [{ from: "Project1App", to: `${projectNameCapital}App` }],
  });

  const { auManufacturer, auSubtype } = options;

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: [
      "wrapper/auv3-xcode-project/Project1/Model/AudioUnitHostModel.swift",
    ],
    replacements: [
      {
        from: `subType: String = "pj42"`,
        to: `subType: String = "${auSubtype}"`,
      },
      {
        from: `manufacturer: String = "Myco"`,
        to: `manufacturer: String = "${auManufacturer}"`,
      },
    ],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["wrapper/auv3-xcode-project/Project1Extension/info.plist"],
    replacements: [
      {
        from: "<string>pj42</string>",
        to: `<string>${auSubtype}</string>`,
      },
      {
        from: "<string>Myco</string>",
        to: `<string>${auManufacturer}</string>`,
      },
      {
        from: "<string>Project1Extension</string>",
        to: `<string>${extensionNameCapital}</string>`,
      },
    ],
  });

  workerHelper_updateFileNamesWithPrefix(projectFolderPath, {
    filePaths: [
      "wrapper/auv3-xcode-project/Project1/Project1App.swift",
      "wrapper/auv3-xcode-project/Project1/Project1.entitlements",
      "wrapper/auv3-xcode-project/Project1",
      "wrapper/auv3-xcode-project/Project1.xcodeproj",
      "wrapper/auv3-xcode-project/Project1Extension",
    ],
    originalPrefix: "Project1",
    newPrefix: projectNameCapital,
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

  const hasVst3 = options.platforms.includes("vst3");
  const hasClap = options.platforms.includes("clap");

  const hasVstDevHost = hasVst3 && options.includeDevHosts;
  const hasClapDevHost = hasClap && options.includeDevHosts;

  if (!hasVstDevHost) {
    removeConditionalLine(
      `add_subdirectory(\${SONIC_ROOT_DIR}/hosts/vst-dev-host/vst_dev_host
                 \${CMAKE_CURRENT_BINARY_DIR}/vst_dev_host)`,
    );
  }
  if (!hasClapDevHost) {
    removeConditionalLine(
      `add_subdirectory(\${SONIC_ROOT_DIR}/hosts/clap-dev-host/clap_dev_host
                 \${CMAKE_CURRENT_BINARY_DIR}/clap_dev_host)`,
    );
  }

  if (!hasVst3) {
    removeConditionalLine(`add_subdirectory(wrapper/vst3)`);
  }
  if (!hasClap) {
    removeConditionalLine(`add_subdirectory(wrapper/clap)`);
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

    const projectNameKebab = casingToKebab(projectName);
    workerHelper_replaceStrings(projectFolderPath, {
      filePaths: ["run.sh"],
      replacements: [
        { from: "project1-vst3.vst3", to: `${projectNameKebab}-vst3.vst3` },
        { from: "project1-clap.clap", to: `${projectNameKebab}-clap.clap` },
      ],
    });
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
  renamePluginSourceFiles(taskContext);
  if (options.platforms.includes("vst3")) {
    addVstWrapper(taskContext);
  }
  if (options.platforms.includes("clap")) {
    addClapWrapper(taskContext);
  }
  if (options.platforms.includes("auv3")) {
    addAuv3XcodeProject(taskContext);
  }
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
