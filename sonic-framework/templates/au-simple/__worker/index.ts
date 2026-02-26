import path from "path";
import {
	casingToCapital,
	workerHelper_copyProjectContent_excludingWorkerFolder,
	workerHelper_replaceStrings,
	workerHelper_updateFileNamesWithPrefix,
} from "../../../common/worker-helper";
import type { WorkerInterface } from "../../../common/worker-types";

const worker: WorkerInterface = {
	async createProject(projectName, templateName) {
		workerHelper_copyProjectContent_excludingWorkerFolder(
			projectName,
			templateName,
		);

		const newFolderPath = path.join(process.cwd(), projectName);

		const projectNameCapital = casingToCapital(projectName);

		const extensionNameCapital = `${projectNameCapital}Extension`;

		workerHelper_updateFileNamesWithPrefix(newFolderPath, {
			filePaths: [
				"Project1",
				"Project1.xcodeproj",
				"Project1/Project1App.swift",
				"Project1/Project1.entitlements",
				"Project1Extension",
				"Project1Extension/Project1Extension-Bridging-Header.h",
			],
			originalPrefix: "Project1",
			newPrefix: projectNameCapital,
		});

		workerHelper_replaceStrings(newFolderPath, {
			filePaths: ["Project1/Project1App.swift"],
			replacements: [{ from: "Project1App", to: `${projectNameCapital}App` }],
		});

		//todo: generate random subtype
		const auSubtype = "aaaa";
		const auManufacture = "Myco";

		workerHelper_replaceStrings(newFolderPath, {
			filePaths: ["Project1/Model/AudioUnitHostModel.swift"],
			replacements: [
				{ from: "__TEMPLATE_AU_SUBTYPE__", to: auSubtype },
				{ from: "__TEMPLATE_AU_MANUFACTURER__", to: auManufacture },
			],
		});

		workerHelper_replaceStrings(newFolderPath, {
			filePaths: ["Project1Extension/info.plist"],
			replacements: [
				{ from: "__TEMPLATE_AU_SUBTYPE__", to: auSubtype },
				{ from: "__TEMPLATE_AU_MANUFACTURER__", to: auManufacture },
				{ from: "__TEMPLATE_AU_EXTENSION_NAME__", to: extensionNameCapital },
			],
		});

		return true;
	},
};
export default worker;
