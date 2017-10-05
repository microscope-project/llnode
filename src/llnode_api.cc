#include <lldb/API/SBCommandReturnObject.h>
#include <lldb/API/SBCompileUnit.h>
#include <lldb/API/SBDebugger.h>
#include <lldb/API/SBFileSpec.h>
#include <lldb/API/SBFrame.h>
#include <lldb/API/SBModule.h>
#include <lldb/API/SBProcess.h>
#include <lldb/API/SBSymbol.h>
#include <lldb/API/SBTarget.h>
#include <lldb/API/SBThread.h>
#include <lldb/lldb-enumerations.h>

#include "src/llnode.h"
#include "src/llscan.h"
#include "src/llv8.h"
#include "string.h"

namespace llnode {

static bool loaded = false;

static lldb::SBDebugger debugger;
static lldb::SBTarget target;
static lldb::SBProcess process;

static LLScan llscan;
static std::vector<TypeRecord*> object_types;

llnode::v8::LLV8 llv8;

/* Initialize the SB API and load the core dump */
int initSBTarget(char* filename, char* executable) {
  if (!loaded) {
    lldb::SBDebugger::Initialize();
    debugger = lldb::SBDebugger::Create();
    loaded = true;
    // fprintf(stdout,"llnode_api.cc: SB API initialized\n");
  }

  // Single instance target for now
  target = debugger.CreateTarget(executable);
  if (!target.IsValid()) {
    // fprintf(stdout, "Target created with %s is invalid\n", executable);
    return -1;
  }

  process = target.LoadCore(filename);

  // Load V8 constants from postmortem data
  llv8.Load(target);
  // fprintf(stdout,"llnode_api.cc: SB loaded code dump %s\n", filename);
  return 0;
}

int getSBProcessInfo(int buffer_size, char* buffer) {
  lldb::SBStream info;
  char* cursor = buffer;

  process.GetDescription(info);
  cursor += sprintf(cursor, "%s", info.GetData());
  return 0;
}

int getSBProcessID() { return process.GetProcessID(); }

const char* getSBProcessState() {
  int state = process.GetState();

  switch (state) {
    case lldb::StateType::eStateInvalid:
      return "invalid";
    case lldb::StateType::eStateUnloaded:
      return "unloaded";
    case lldb::StateType::eStateConnected:
      return "connected";
    case lldb::StateType::eStateAttaching:
      return "attaching";
    case lldb::StateType::eStateLaunching:
      return "launching";
    case lldb::StateType::eStateStopped:
      return "stopped";
    case lldb::StateType::eStateRunning:
      return "running";
    case lldb::StateType::eStateStepping:
      return "stepping";
    case lldb::StateType::eStateCrashed:
      return "crashed";
    case lldb::StateType::eStateDetached:
      return "detached";
    case lldb::StateType::eStateExited:
      return "exited";
    case lldb::StateType::eStateSuspended:
      return "suspended";
    default:
      return "unknown";
  }
}

int getSBThreadCount() { return process.GetNumThreads(); }

int getSBFrameCount(int threadIndex) {
  lldb::SBThread thread = process.GetThreadAtIndex(threadIndex);
  return thread.GetNumFrames();
}

int getSBFrame(int threadIndex, int frameIndex, int buffer_size, char* buffer) {
  lldb::SBThread thread = process.GetThreadAtIndex(threadIndex);
  lldb::SBFrame frame = thread.GetFrameAtIndex(frameIndex);
  lldb::SBSymbol symbol = frame.GetSymbol();

  char* cursor = buffer;
  if (symbol.IsValid()) {
    cursor += sprintf(cursor, "Native: ");
    cursor += sprintf(cursor, "%s", frame.GetFunctionName());
    lldb::SBModule module = frame.GetModule();
    lldb::SBFileSpec moduleFileSpec = module.GetFileSpec();
    cursor += sprintf(cursor, " [%s/%s]", moduleFileSpec.GetDirectory(),
                      moduleFileSpec.GetFilename());
    lldb::SBCompileUnit compileUnit = frame.GetCompileUnit();
    lldb::SBFileSpec compileUnitFileSpec = compileUnit.GetFileSpec();
    if (compileUnitFileSpec.GetDirectory() != NULL ||
        compileUnitFileSpec.GetFilename() != NULL) {
      cursor +=
          sprintf(cursor, "\n\t [%s: %s]", compileUnitFileSpec.GetDirectory(),
                  compileUnitFileSpec.GetFilename());
    }
  } else {
    // V8 frame
    llnode::v8::Error err;
    llnode::v8::JSFrame v8_frame(&llv8, static_cast<int64_t>(frame.GetFP()));
    std::string res = v8_frame.Inspect(true, err);

    // Skip invalid frames
    // fprintf(stdout,"JS string is [%s]\n",res.c_str());
    if (err.Fail() || strlen(res.c_str()) == 0 ||
        strncmp(res.c_str(), "<", 1) == 0) {
      if (strncmp(res.c_str(), "<", 1) == 0) {
        cursor += sprintf(cursor, "Unknown: %s", res.c_str());
      } else {
        cursor += sprintf(cursor, "???");
      }
    } else {
      // V8 symbol
      cursor += sprintf(cursor, "JavaScript: %s", res.c_str());
    }
  }
  return 0;
}

int getSBTypeCount() {
  lldb::SBCommandReturnObject result;
  /* Initial scan to create the JavaScript object map */
  if (!llscan.ScanHeapForObjects(target, result)) {
    return 0;
  }

  // Load the object types into a vector
  TypeRecordMap::iterator end = llscan.GetMapsToInstances().end();
  object_types.clear();
  for (TypeRecordMap::iterator it = llscan.GetMapsToInstances().begin();
       it != end; ++it) {
    object_types.push_back(it->second);
  }

  // Sort by instance count
  std::sort(object_types.begin(), object_types.end(),
            TypeRecord::CompareInstanceCounts);
  return object_types.size();
}

int getSBTypeName(int typeIndex, int buffer_size, char* buffer) {
  TypeRecord* type = object_types.at(typeIndex);
  char* cursor = buffer;

  std::string typeName = type->GetTypeName();
  cursor += sprintf(cursor, "%s", typeName.c_str());
  return 0;
}

int getSBTypeInstanceCount(int typeIndex) {
  TypeRecord* type = object_types.at(typeIndex);
  return type->GetInstanceCount();
}

int getSBTypeTotalSize(int typeIndex) {
  TypeRecord* type = object_types.at(typeIndex);
  return type->GetTotalInstanceSize();
}

std::set<uint64_t>& getSBTypeInstances(int typeIndex) {
  TypeRecord* type = object_types.at(typeIndex);
  return type->GetInstances();
}

int getSBObject(uint64_t address, int buffer_size, char* buffer) {
  // fprintf(stderr, "llnode_api.cc getSBObject() called for address %llx\n",
  //         address);
  v8::Value v8_value(&llv8, address);
  v8::Value::InspectOptions inspect_options;
  inspect_options.detailed = true;
  inspect_options.length = 16;

  v8::Error err;
  strcpy(buffer, v8_value.Inspect(&inspect_options, err).c_str());
  // fprintf(stderr, "llnode_api.cc getSBObject() returning %s\n", buffer);
  return 0;
}
}  // namespace llnode
