import path from "path";
import {
	casingToCapital,
	casingToSnake,
	workerHelper_copyProjectContent_excludingWorkerFolder,
	workerHelper_replaceStrings,
	workerHelper_updateFileNamesWithPrefix,
} from "../../../common/worker-helper";
import type { WorkerInterface } from "../../../common/worker-types";

export const template_vstSimple_worker: WorkerInterface = {
	async createProject(projectName, templateName) {
		workerHelper_copyProjectContent_excludingWorkerFolder(
			projectName,
			templateName,
		);

		const newFolderPath = path.join(process.cwd(), projectName);

		const projectNameSnake = casingToSnake(projectName);
		const projectNameCapital = casingToCapital(projectName);

		workerHelper_updateFileNamesWithPrefix(newFolderPath, {
			filePaths: [
				"source/vst/project1_controller.cpp",
				"source/vst/project1_controller.h",
				"source/vst/project1_processor.cpp",
				"source/vst/project1_processor.h",
			],
			originalPrefix: "project1",
			newPrefix: projectNameSnake,
		});

		workerHelper_replaceStrings(newFolderPath, {
			filePaths: [
				"source/vst/plugin_factory.cpp",
				"source/vst/project1_controller.cpp",
				"source/vst/project1_controller.h",
				"source/vst/project1_processor.cpp",
				"source/vst/project1_processor.h",
				"Project1Synthesizer.cpp",
				"Project1Synthesizer.h",
			],
			replacements: [
				{ from: "project1_controller", to: `${projectNameSnake}_controller` },
				{ from: "project1_processor", to: `${projectNameSnake}_processor` },
				{ from: "Project1Controller", to: `${projectNameCapital}Controller` },
				{ from: "Project1Processor", to: `${projectNameCapital}Processor` },
				{ from: "Project1Synthesizer", to: `${projectNameCapital}Synthesizer` },
				{ from: "namespace Project1", to: `namespace ${projectNameCapital}` },
			],
		});

		workerHelper_replaceStrings(newFolderPath, {
			filePaths: ["source/vst/version.h"],
			replacements: [{ from: "Project1", to: projectNameCapital }],
		});

		const processorCID = crypto.randomUUID();
		const controllerCID = crypto.randomUUID();

		workerHelper_replaceStrings(newFolderPath, {
			filePaths: ["source/vst/plugin_factory.cpp"],
			replacements: [
				{ from: "MyPlugin", to: projectNameCapital },
				{ from: "__TEMPLATE_VST3_PROCESSOR_CID__", to: processorCID },
				{ from: "__TEMPLATE_VST3_CONTROLLER_CID__", to: controllerCID },
			],
		});
		return true;
	},
};
