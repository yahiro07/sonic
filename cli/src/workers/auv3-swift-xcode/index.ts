import {
  TemplateWorker,
  casingToCapital,
  generateRandomString,
  workerHelper_copyProjectContentFiles,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
  workerHelper_getNewProjectFolderPath,
  workerHelper_copyProjectContentFiles_withRenaming,
} from "@/src/common";
import * as clackPrompts from "@clack/prompts";

type TemplateOptions = {
  auManufacturer: string;
  auSubtype: string;
};

async function readTemplateOptions(): Promise<TemplateOptions | "cancelled"> {
  let auManufacturer: string | symbol = "";
  let auSubtype: string | symbol = "";

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

  return {
    auManufacturer,
    auSubtype,
  };
}

type TaskContext = {
  projectName: string;
  templateName: string;
  options: TemplateOptions;
  projectFolderPath: string;
};

function instantiateProject({
  projectName,
  templateName,
  options,
  projectFolderPath,
}: TaskContext) {
  workerHelper_copyProjectContentFiles(projectName, templateName, [
    "Project1",
    "Project1.xcodeproj/project.pbxproj",
    "Project1Extension/Common",
    "Project1Extension/DSP",
    "Project1Extension/Root",
    "Project1Extension/Info.plist",
    "README.md",
  ]);
  workerHelper_copyProjectContentFiles_withRenaming(projectName, templateName, [
    { from: ".gitignore_template", to: ".gitignore" },
  ]);

  const projectNameCapital = casingToCapital(projectName);

  const extensionNameCapital = `${projectNameCapital}Extension`;

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1/Project1App.swift"],
    replacements: [{ from: "Project1App", to: `${projectNameCapital}App` }],
  });

  const { auSubtype, auManufacturer } = options;

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1/Model/AudioUnitHostModel.swift"],
    replacements: [
      { from: `"prj1"`, to: `"${auSubtype}"` },
      { from: `"Myco"`, to: `"${auManufacturer}"` },
    ],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1Extension/info.plist"],
    replacements: [
      {
        from: "<string>prj1</string>",
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

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1.xcodeproj/project.pbxproj"],
    replacements: [
      {
        from: "Project1",
        to: projectNameCapital,
      },
    ],
  });

  workerHelper_updateFileNamesWithPrefix(projectFolderPath, {
    filePaths: [
      "Project1/Project1App.swift",
      "Project1/Project1.entitlements",
      "Project1",
      "Project1.xcodeproj",
      "Project1Extension/Common/Project1Extension-Bridging-Header.h",
      "Project1Extension",
    ],
    originalPrefix: "Project1",
    newPrefix: projectNameCapital,
  });

  return true;
}

function createTemplateWorker(): TemplateWorker {
  return {
    async createProject(projectName, templateName) {
      const templateOptions = await readTemplateOptions();
      if (templateOptions === "cancelled") {
        return "cancelled";
      }
      const context: TaskContext = {
        projectName,
        templateName,
        options: templateOptions,
        projectFolderPath: workerHelper_getNewProjectFolderPath(projectName),
      };
      instantiateProject(context);
      return true;
    },
  };
}

export default {
  name: "auv3-swift-xcode",
  description: "AUv3 template with Swift and Xcode",
  worker: createTemplateWorker(),
};
