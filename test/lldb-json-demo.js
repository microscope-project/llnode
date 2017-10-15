const { fromCoredump } = require('../');

if (process.argv.length < 4) {
  throw new Error('Usage: node lldb-json-demo.js executable core');
}

const executable = process.argv[2];
const dump = process.argv[3];

console.log('============= Loading ==============')
console.log(`Loading dump: ${dump}, executable: ${executable}`);
const llnode = fromCoredump(dump, executable);

console.log('=============== Process Info ============')
console.log('Process info', llnode.getProcessInfo());
console.log('=============== Process Object ============')
console.log('Process object', llnode.getProcessObject());
console.log('============= Heap Types ==============')
console.log('Heap types', llnode.getHeapTypes());

console.log('============ Thread & Frames ===============')
const threadCount = llnode.getThreadCount();
console.log('Thread count', threadCount);

for (let i = 0; i < threadCount; ++i) {
  const frames = llnode.getFrameCount(i);
  console.log(`  Thread ${i} has ${frames} frames:`);
  for (let j = 0; j < frames; ++j) {
    console.log(`    #${j} ${llnode.getFrame(i, j)}`);
  }
}

console.log('=========== Type API ================')
const typeCount = llnode.getTypeCount();
console.log('Type Count', typeCount);

for (let i = 0; i < typeCount; ++i) {
  const name = llnode.getTypeName(i);
  const instanceCount = llnode.getTypeInstanceCount(i);
  const totalSize = llnode.getTypeTotalSize(i);
  console.log(`  #${i} ${name}: ${instanceCount} instances, ` +
              `total size: ${totalSize}`);
}

console.log('============== Object API ============')
const typeInstance = llnode.getHeapTypes()[0].nextInstance();
console.log(`Type instance of type 0: 0x${(typeInstance.address).toString(16)}`);
console.log(llnode.getObjectAtAddress(typeInstance.address));