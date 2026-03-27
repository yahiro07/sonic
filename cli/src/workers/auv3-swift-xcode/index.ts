import {
  TemplateWorker,
  casingToCapital,
  generateRandomString,
  workerHelper_copyProjectContentFiles,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
  workerHelper_getNewProjectFolderPath,
} from "@/src/common";

function createTemplateWorker(): TemplateWorker {
  return {
    async createProject(projectName, templateName) {
      workerHelper_copyProjectContentFiles(projectName, templateName, [
        "Project1",
        "Project1.xcodeproj/project.pbxproj",
        "Project1Extension/Common",
        "Project1Extension/DSP",
        "Project1Extension/Root",
        "Project1Extension/Info.plist",
        "README.md",
      ]);

      const newFolderPath = workerHelper_getNewProjectFolderPath(projectName);

      const projectNameCapital = casingToCapital(projectName);

      const extensionNameCapital = `${projectNameCapital}Extension`;

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: ["Project1/Project1App.swift"],
        replacements: [{ from: "Project1App", to: `${projectNameCapital}App` }],
      });

      const auSubtype = generateRandomString("alphaNumeric", 4);
      const auManufacture = "Myco";

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: ["Project1/Model/AudioUnitHostModel.swift"],
        replacements: [
          { from: `"prj1"`, to: `"${auSubtype}"` },
          { from: `"Myco"`, to: `"${auManufacture}"` },
        ],
      });

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: ["Project1Extension/info.plist"],
        replacements: [
          {
            from: "<string>prj1</string>",
            to: `<string>${auSubtype}</string>`,
          },
          {
            from: "<string>Myco</string>",
            to: `<string>${auManufacture}</string>`,
          },
          {
            from: "<string>Project1Extension</string>",
            to: `<string>${extensionNameCapital}</string>`,
          },
        ],
      });

      // workerHelper_replaceStrings(newFolderPath, {
      // 	filePaths: ["project.toml"],
      // 	replacements: [
      // 		{ from: "__TEMPLATE_AU_SUBTYPE__", to: auSubtype },
      // 		{ from: "__TEMPLATE_AU_MANUFACTURER__", to: auManufacture },
      // 	],
      // });

      workerHelper_updateFileNamesWithPrefix(newFolderPath, {
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
    },
  };
}

export default {
  name: "auv3-swift-xcode",
  description: "AUv3 template with Swift and Xcode",
  worker: createTemplateWorker(),
};
