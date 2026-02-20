#!/usr/bin/env node

import { dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

console.log('Sonic Framework CLI');
const args = process.argv.slice(2);
console.log('Running with arguments:', args);

if (args[0] === 'dev') {
  console.log('Starting development mode...');
} else {
  console.log('Available commands: dev');
}
