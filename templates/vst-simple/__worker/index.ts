import {
  TemplateWorker,
  casingToCapital,
  casingToSnake,
  workerHelper_copyProjectContents_withWhiteList,
  workerHelper_getNewProjectFolderPath,
  workerHelper_replaceStrings,
  workerHelper_updateFileNamesWithPrefix,
} from "@/common";

function createTemplateWorker(): TemplateWorker {
  return {
    async createProject(projectName, templateName) {
      console.log("vst-simple worker");
      workerHelper_copyProjectContents_withWhiteList(
        projectName,
        templateName,
        [
          "external",
          "frontend",
          // "libs",  //skip copying, it's downloaded from github with FetchContent
          "resource",
          "source",
          ".gitignore",
          "CMakeLists.txt",
          "README.md",
        ],
      );

      const newFolderPath = workerHelper_getNewProjectFolderPath(projectName);

      const projectNameSnake = casingToSnake(projectName);
      const projectNameCapital = casingToCapital(projectName);

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: [
          "source/project1_synthesizer.cpp",
          "source/project1_synthesizer.h",
        ],
        replacements: [
          {
            from: "Project1Synthesizer",
            to: `${projectNameCapital}Synthesizer`,
          },
          {
            from: "project1_synthesizer.h",
            to: `${projectNameSnake}_synthesizer.h`,
          },
        ],
      });

      workerHelper_updateFileNamesWithPrefix(newFolderPath, {
        filePaths: [
          "source/project1_synthesizer.cpp",
          "source/project1_synthesizer.h",
        ],
        originalPrefix: "project1",
        newPrefix: projectNameSnake,
      });

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: ["source/version.h"],
        replacements: [{ from: "Project1", to: projectNameCapital }],
      });

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: ["CMakeLists.txt"],
        replacements: [
          {
            from: "project1_synthesizer.cpp",
            to: `${projectNameSnake}_synthesizer.cpp`,
          },
          {
            from: "Project1",
            to: projectNameCapital,
          }
        ],
      });

      const processorCID = crypto.randomUUID().toUpperCase();
      const controllerCID = crypto.randomUUID().toUpperCase();

      workerHelper_replaceStrings(newFolderPath, {
        filePaths: ["source/plugin_factory.cpp"],
        replacements: [
          {
            from: "project1_synthesizer.h",
            to: `${projectNameSnake}_synthesizer.h`,
          },
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
