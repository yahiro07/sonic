import type { TemplateWorker } from "../../../packages/cli/common";
import {
  casingToCapital,
  generateRandomString,
  workerHelper_copyProjectContents_withWhiteList,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
  workerHelper_getNewProjectFolderPath,
} from "../../../packages/cli/common";
import fileEntries from "./file-entries.json";

function createTemplateWorker(): TemplateWorker {
	return {
		async createProject(projectName, templateName) {
			workerHelper_copyProjectContents_withWhiteList(
				projectName,
				templateName,
				fileEntries,
			);

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
	description: "Audio Unit Simple Template",
	worker: createTemplateWorker(),
};
