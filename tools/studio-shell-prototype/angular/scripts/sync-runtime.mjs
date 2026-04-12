import { cp, mkdir, rm } from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const currentFilePath = fileURLToPath(import.meta.url);
const scriptDirectory = path.dirname(currentFilePath);
const angularRoot = path.resolve(scriptDirectory, '..');
const runtimeRoot = path.resolve(angularRoot, '..', 'runtime');
const distRoot = path.resolve(angularRoot, 'dist', 'coretools-angular', 'browser');

async function syncRuntime() {
  await rm(runtimeRoot, { recursive: true, force: true });
  await mkdir(runtimeRoot, { recursive: true });
  await cp(distRoot, runtimeRoot, { recursive: true });
  console.log(`CoreTools runtime synced to ${runtimeRoot}`);
}

syncRuntime().catch((error) => {
  console.error('Failed to sync the CoreTools runtime output.', error);
  process.exitCode = 1;
});
