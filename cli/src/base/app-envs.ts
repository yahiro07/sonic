import fs from "fs";
import path from "path";

const isWorkerDev = process.env.CLI_DEBUG_TEMPLATE_WORKERS;
if (isWorkerDev) {
  console.log("isWorkerDev: true");
}
export const appEnvs = {
  isWorkerDev,
};

export function appEnvs_getPackageRootFolderPath() {
  const entryPath = fs.realpathSync(process.argv[1]);
  const binFolderPath = path.dirname(entryPath);
  if (appEnvs.isWorkerDev) {
    //from src/sonic.ts
    return path.join(binFolderPath, "../");
  } else {
    //from bin/sonic.js
    return path.join(binFolderPath, "../");
  }
}

export function appEnvs_getTemplatesFolderPath() {
  const packageRootFolderPath = appEnvs_getPackageRootFolderPath();
  if (appEnvs.isWorkerDev) {
    //for development, refer <sonic_repo_root>/templates directly
    return path.join(packageRootFolderPath, "../templates");
  } else {
    //for production templates are bundled in the package
    return path.join(packageRootFolderPath, "templates");
  }
}
