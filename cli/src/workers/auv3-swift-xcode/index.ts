import {
  TemplateWorker,
  casingToCapital,
  generateRandomString,
  workerHelper_copyProjectContentFiles,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
  workerHelper_getNewProjectFolderPath,
  workerHelper_copyProjectContentFiles_withRenaming,
  workerHelper_buildFrontend,
} from "@/src/common";
import * as clackPrompts from "@clack/prompts";

type TemplateOptions = {
  frontendVariant: "react" | "vanilla_minimum";
  auOrganizationIdentifier: string;
  auManufacturer: string;
  auSubtype: string;
  buildFrontend: boolean;
};

async function readTemplateOptions(
  projectName: string,
): Promise<TemplateOptions | "cancelled"> {
  const _frontendVariant = await clackPrompts.select({
    message: "Select frontend variation",
    options: [
      { value: "react", label: "React" },
      { value: "vanilla_minimum", label: "Vanilla Minimum" },
    ],
    initialValue: "react",
  });
  if (clackPrompts.isCancel(_frontendVariant)) {
    return "cancelled";
  }
  const frontendVariant =
    _frontendVariant as TemplateOptions["frontendVariant"];

  let auOrganizationIdentifier: string | symbol = "";
  let auManufacturer: string | symbol = "";
  let auSubtype: string | symbol = "";
  const auOrganizationIdentifierDefault = `com.example.sonic`;
  auOrganizationIdentifier = await clackPrompts.text({
    message: `Organization Identifier`,
    placeholder: auOrganizationIdentifierDefault,
  });
  if (clackPrompts.isCancel(auOrganizationIdentifier)) {
    return "cancelled";
  }
  if (auOrganizationIdentifier == "") {
    console.log("   " + auOrganizationIdentifierDefault);
    auOrganizationIdentifier = auOrganizationIdentifierDefault;
  }
  const auManufacturerDefault = "Myco";
  auManufacturer = await clackPrompts.text({
    message: `AUv3 Manufacturer Code`,
    placeholder: auManufacturerDefault,
  });
  if (clackPrompts.isCancel(auManufacturer)) {
    return "cancelled";
  }
  if (auManufacturer == "") {
    console.log("   " + auManufacturerDefault);
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
    console.log("   " + auSubtypeDefault);
    auSubtype = auSubtypeDefault;
  }

  let buildFrontend = false;
  if (frontendVariant === "react") {
    const _buildFrontend = await clackPrompts.confirm({
      message: "Build frontend now?",
      initialValue: true,
    });
    if (clackPrompts.isCancel(_buildFrontend)) {
      return "cancelled";
    }
    buildFrontend = _buildFrontend;
  }
  return {
    frontendVariant,
    auOrganizationIdentifier,
    auManufacturer,
    auSubtype,
    buildFrontend,
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
    "Project1.xcodeproj/xcshareddata",
    "Project1Extension/Api",
    "Project1Extension/Common",
    "Project1Extension/DSP",
    "Project1Extension/Root",
    "Project1Extension/Info.plist",
    "Project1Extension/Project1Extension.entitlements",
    "README.md",
  ]);
  workerHelper_copyProjectContentFiles_withRenaming(projectName, templateName, [
    { from: ".gitignore_template", to: ".gitignore" },
  ]);

  if (options.frontendVariant === "react") {
    workerHelper_copyProjectContentFiles(projectName, "_common", ["frontend"]);
  } else if (options.frontendVariant === "vanilla_minimum") {
    workerHelper_copyProjectContentFiles(projectName, "_common", ["pages"]);
    workerHelper_replaceStrings(projectFolderPath, {
      filePaths: ["Project1Extension/Root/MainContentView.swift"],
      replacements: [
        {
          from: `webViewIo.loadURL("app://www-bundles/index.html")`,
          to: `webViewIo.loadURL("app://www-vanilla/index.html")`,
        },
      ],
    });
  }

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
      { from: `"prj2"`, to: `"${auSubtype}"` },
      { from: `"Myco"`, to: `"${auManufacturer}"` },
    ],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1Extension/info.plist"],
    replacements: [
      {
        from: "<string>prj2</string>",
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

  const mainBundleIdentifier = `${options.auOrganizationIdentifier}.${projectNameCapital}`;
  const extensionBundleIdentifier = `${mainBundleIdentifier}.${extensionNameCapital}`;
  const appGroupIdentifier = `group.${mainBundleIdentifier}`;

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1.xcodeproj/project.pbxproj"],
    replacements: [
      {
        from: "Project1",
        to: projectNameCapital,
      },
      {
        from: "com.example.sonic.auv3-swift-xcode.extension",
        to: extensionBundleIdentifier,
      },
      {
        from: "com.example.sonic.auv3-swift-xcode",
        to: mainBundleIdentifier,
      },
    ],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: [
      "Project1.xcodeproj/xcshareddata/xcschemes/Project1.xcscheme",
      "Project1.xcodeproj/xcshareddata/xcschemes/Project1Extension.xcscheme",
    ],
    replacements: [{ from: "Project1", to: projectNameCapital }],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: [
      "Project1/Project1.entitlements",
      "Project1Extension/Project1Extension.entitlements",
    ],
    replacements: [
      {
        from: `<string>group.com.example.sonic.auv3-swift-xcode</string>`,
        to: `<string>${appGroupIdentifier}</string>`,
      },
    ],
  });

  workerHelper_replaceStrings(projectFolderPath, {
    filePaths: ["Project1Extension/Common/AudioUnitViewController.swift"],
    replacements: [
      {
        from: `SharedContainer.setAppGroupId("group.com.example.sonic.auv3-swift-xcode")`,
        to: `SharedContainer.setAppGroupId("${appGroupIdentifier}")`,
      },
    ],
  });

  workerHelper_updateFileNamesWithPrefix(projectFolderPath, {
    filePaths: [
      "Project1/Project1App.swift",
      "Project1/Project1.entitlements",
      "Project1",
      "Project1.xcodeproj/xcshareddata/xcschemes/Project1.xcscheme",
      "Project1.xcodeproj/xcshareddata/xcschemes/Project1Extension.xcscheme",
      "Project1.xcodeproj",
      "Project1Extension/Common/Project1Extension-Bridging-Header.h",
      "Project1Extension/Project1Extension.entitlements",
      "Project1Extension",
    ],
    originalPrefix: "Project1",
    newPrefix: projectNameCapital,
  });

  if (options.frontendVariant === "react") {
    workerHelper_copyProjectContentFiles(projectName, "_common", ["frontend"]);
    if (options.buildFrontend) {
      workerHelper_buildFrontend(projectFolderPath, "frontend");
    }
  } else {
    workerHelper_copyProjectContentFiles(projectName, "_common", ["pages"]);
  }

  return true;
}

function createTemplateWorker(): TemplateWorker {
  return {
    async createProject(projectName, templateName) {
      const templateOptions = await readTemplateOptions(projectName);
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
