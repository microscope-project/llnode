const ll = require('../');

if (process.argv.length < 4) {
  throw new Error('Usage: node lldb-json-demo.js executable core');
}

const executable = process.argv[2];
const dump = process.argv[3];

console.log('============= Loading ==============')
console.log(`Loading dump: ${dump}, executable: ${executable}`);
ll.loadDump(dump, executable);

console.log('=============== Process Info ============')
console.log('Process info', ll.getProcessInfo());
console.log('=============== Process Object ============')
console.log('Process object', ll.getProcessObject());
console.log('============= Heap Types ==============')
console.log('Heap types', ll.getHeapTypes());

console.log('============ Thread & Frames ===============')
const threadCount = ll.getThreadCount();
console.log('Thread count', threadCount);

for (let i = 0; i < threadCount; ++i) {
  const frames = ll.getFrameCount(i);
  console.log(`  Thread ${i} has ${frames} frames:`);
  for (let j = 0; j < frames; ++j) {
    console.log(`    #${j} ${ll.getFrame(i, j)}`);
  }
}

console.log('=========== Type API ================')
const typeCount = ll.getTypeCount();
console.log('Type Count', typeCount);

for (let i = 0; i < typeCount; ++i) {
  const name = ll.getTypeName(i);
  const instanceCount = ll.getTypeInstanceCount(i);
  const totalSize = ll.getTypeTotalSize(i);
  console.log(`  #${i} ${name}: ${instanceCount} instances, ` +
              `total size: ${totalSize}`);
}

console.log('============== Object API ============')
const typeInstance = ll.getHeapTypes()[0].nextInstance();
console.log(`Type instance of type 0: 0x${(typeInstance.address).toString(16)}`);
console.log(ll.getObjectAtAddress(typeInstance.address));