import path from "path";
import type { TemplateWorker } from "../../../common";
import {
	casingToCapital,
	casingToSnake,
	workerHelper_copyProjectContents_withWhiteList,
	workerHelper_replaceStrings,
	workerHelper_updateFileNamesWithPrefix,
} from "../../../common";
import fileEntries from "./file-entries.json";

function createTemplateWorker(): TemplateWorker {
	return {
		async createProject(projectName, templateName) {
			workerHelper_copyProjectContents_withWhiteList(
				projectName,
				templateName,
				fileEntries,
			);

			const newFolderPath = path.join(process.cwd(), projectName);

			const projectNameSnake = casingToSnake(projectName);
			const projectNameCapital = casingToCapital(projectName);

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
					{
						from: "Project1Synthesizer",
						to: `${projectNameCapital}Synthesizer`,
					},
					{ from: "namespace Project1", to: `namespace ${projectNameCapital}` },
				],
			});

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
				filePaths: ["source/vst/version.h"],
				replacements: [{ from: "Project1", to: projectNameCapital }],
			});

			const processorCID = crypto.randomUUID().toUpperCase();
			const controllerCID = crypto.randomUUID().toUpperCase();

			workerHelper_replaceStrings(newFolderPath, {
				filePaths: ["source/vst/plugin_factory.cpp"],
				replacements: [
					{ from: "MyPlugin", to: projectNameCapital },
					{ from: "E458F80E-DEED-40DB-AD59-2C739A7DA7A0", to: processorCID },
					{ from: "6BAD2674-0204-4522-8971-58C6296A4552", to: controllerCID },
				],
			});
			return true;
		},
	};
}
export default {
	name: "vst-simple",
	description: "VST Simple Template",
	worker: createTemplateWorker(),
};
