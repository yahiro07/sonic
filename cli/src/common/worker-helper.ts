import { appEnvs_getTemplatesFolderPath } from "@/src/base/app-envs";
import { execSync } from "child_process";
import fs from "fs";
import path from "path";

export function workerHelper_getNewProjectFolderPath(projectName: string) {
  return path.join(process.cwd(), projectName);
}

export function workerHelper_getTemplateFolderPath(templateName: string) {
  const templatesFolderPath = appEnvs_getTemplatesFolderPath();
  return path.join(templatesFolderPath, templateName);
}

export function workerHelper_copyFile(srcPath: string, destPath: string) {
  if (!fs.existsSync(srcPath)) {
    throw new Error(`source file ${srcPath} does not exist`);
  }
  if (fs.statSync(srcPath).isDirectory()) {
    fs.cpSync(srcPath, destPath, {
      recursive: true,
    });
  } else {
    fs.copyFileSync(srcPath, destPath);
  }
}

export function workerHelper_copyProjectContentFiles(
  projectName: string,
  templateName: string,
  entries: string[],
) {
  const templateFolderPath = workerHelper_getTemplateFolderPath(templateName);
  const newProjectFolderPath =
    workerHelper_getNewProjectFolderPath(projectName);

  for (const entry of entries) {
    const srcPath = path.join(templateFolderPath, entry);
    const destPath = path.join(newProjectFolderPath, entry);
    if (!fs.existsSync(srcPath)) {
      throw new Error(
        `source entry ${entry} does not exist in template ${templateName}`,
      );
    }
    const destDir = path.dirname(destPath);
    fs.mkdirSync(destDir, { recursive: true });

    if (fs.statSync(srcPath).isDirectory()) {
      fs.cpSync(srcPath, destPath, {
        recursive: true,
      });
    } else {
      fs.copyFileSync(srcPath, destPath);
    }
  }
}

export function workerHelper_copyProjectContentFiles_withRenaming(
  projectName: string,
  templateName: string,
  entries: { from: string; to: string }[],
) {
  const templateFolderPath = workerHelper_getTemplateFolderPath(templateName);
  const newProjectFolderPath =
    workerHelper_getNewProjectFolderPath(projectName);

  for (const entry of entries) {
    const srcPath = path.join(templateFolderPath, entry.from);
    const destPath = path.join(newProjectFolderPath, entry.to);
    if (!fs.existsSync(srcPath)) {
      throw new Error(
        `source entry ${entry.from} does not exist in template ${templateName}`,
      );
    }
    if (fs.statSync(srcPath).isDirectory()) {
      fs.cpSync(srcPath, destPath, {
        recursive: true,
      });
    } else {
      fs.copyFileSync(srcPath, destPath);
    }
  }
}

export function workerHelper_updateFileNamesWithPrefix(
  folderPath: string,
  spec: {
    filePaths: string[];
    originalPrefix: string;
    newPrefix: string;
  },
) {
  const fullPaths = spec.filePaths.map((filePath) =>
    path.join(folderPath, filePath),
  );
  for (const filePath of fullPaths) {
    const fileName = path.basename(filePath);
    if (!fileName.startsWith(spec.originalPrefix)) continue;
    if (!fs.existsSync(filePath)) {
      throw new Error(`file ${filePath} does not exist`);
    }
    const newFileName = fileName.replace(
      new RegExp(spec.originalPrefix, "g"),
      spec.newPrefix,
    );
    const newFilePath = path.join(path.dirname(filePath), newFileName);
    fs.renameSync(filePath, newFilePath);
  }
}

// export function workerHelper_replaceInFileSignaturesWithPrefix(
// 	folderPath: string,
// 	spec: {
// 		filePaths: string[];
// 		replacements: {
// 			originalPrefix: string;
// 			newPrefix: string;
// 		}[];
// 	},
// ) {}

export function workerHelper_replaceStrings(
  folderPath: string,
  spec: {
    filePaths: string[];
    replacements: { from: string; to: string }[];
  },
  checkReplaced = true,
) {
  const replacements = spec.replacements.filter((it) => it.to !== it.from);
  let fromKeysRemaining = new Set(replacements.map((r) => r.from));

  const fullPaths = spec.filePaths.map((filePath) =>
    path.join(folderPath, filePath),
  );
  for (const filePath of fullPaths) {
    if (!fs.existsSync(filePath)) {
      throw new Error(`file ${filePath} does not exist`);
    }
    let fileContent = fs.readFileSync(filePath, "utf-8");
    let replaced = false;
    for (const replacement of replacements) {
      const original = fileContent;
      fileContent = fileContent.replaceAll(replacement.from, replacement.to);
      const thisReplaced = fileContent !== original;
      if (thisReplaced) {
        fromKeysRemaining.delete(replacement.from);
      }
      replaced ||= thisReplaced;
    }
    if (replaced) {
      fs.writeFileSync(filePath, fileContent);
    }
  }
  if (checkReplaced && fromKeysRemaining.size > 0) {
    throw new Error(
      `Some strings to replace were not found in the files. Remaining: ${Array.from(fromKeysRemaining).join(", ")}`,
    );
  }
}

export function workerHelper_removeStringLines(
  folderPath: string,
  spec: {
    filePaths: string[];
    strings: string[];
    keepTrailingNewLineAsIs?: boolean;
  },
  checkReplaced = true,
) {
  workerHelper_replaceStrings(
    folderPath,
    {
      filePaths: spec.filePaths,
      replacements: spec.strings.map((s) => ({
        from: s + (spec.keepTrailingNewLineAsIs ? "" : "\n"),
        to: "",
      })),
    },
    checkReplaced,
  );
}

export function workerHelper_relocateFile(
  folderPath: string,
  spec: { from: string; to: string },
) {
  const fromPath = path.join(folderPath, spec.from);
  const toPath = path.join(folderPath, spec.to);
  if (!fs.existsSync(fromPath)) {
    throw new Error(`file ${fromPath} does not exist`);
  }
  fs.renameSync(fromPath, toPath);
}

export function workerHelper_checkFileExists(
  folderPath: string,
  relativeFilePath: string,
) {
  const filePath = path.join(folderPath, relativeFilePath);
  return fs.existsSync(filePath);
}

export function workerHelper_buildFrontend(
  projectFolderPath: string,
  relativeFrontendFolderPath: string,
) {
  const folderPath = path.join(projectFolderPath, relativeFrontendFolderPath);
  console.log("npm install");
  execSync("npm install", { cwd: folderPath, stdio: "inherit" });
  console.log("npm run build");
  execSync("npm run build", { cwd: folderPath, stdio: "inherit" });
}

export function workerHelper_createFolder(
  projectFolderPath: string,
  relativeFolderPath: string,
) {
  const folderPath = path.join(projectFolderPath, relativeFolderPath);
  fs.mkdirSync(folderPath, { recursive: true });
}
