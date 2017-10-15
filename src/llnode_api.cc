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

#include <cstring>
#include <algorithm>

#include "src/llnode_api.h"
#include "src/llscan.h"
#include "src/llv8.h"

namespace llnode {

LLNodeApi::LLNodeApi()
    : loaded(false),
      debugger(new lldb::SBDebugger()),
      target(new lldb::SBTarget()),
      process(new lldb::SBProcess()),
      llscan(new LLScan()),
      v8_(new llnode::v8::LLV8()) {}
LLNodeApi::~LLNodeApi() = default;
LLNodeApi::LLNodeApi(LLNodeApi&&) = default;
LLNodeApi& LLNodeApi::operator=(LLNodeApi&&) = default;

/* Initialize the SB API and load the core dump */
int LLNodeApi::Init(const char* filename, const char* executable) {
  lldb::SBDebugger::Initialize();  // TODO: should only be called once?

  *debugger = lldb::SBDebugger::Create();
  loaded = true;

  // Single instance target for now
  *target = debugger->CreateTarget(executable);
  if (!target->IsValid()) {
    return -1;
  }

  *process = target->LoadCore(filename);
  // Load V8 constants from postmortem data
  v8_->Load(*target);
  return 0;
}

std::string LLNodeApi::GetProcessInfo() {
  lldb::SBStream info;
  process->GetDescription(info);
  return std::string(info.GetData());
}

int LLNodeApi::GetProcessID() { return process->GetProcessID(); }

std::string LLNodeApi::GetProcessState() {
  int state = process->GetState();

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

int LLNodeApi::GetThreadCount() { return process->GetNumThreads(); }

int LLNodeApi::GetFrameCount(int thread_index) {
  lldb::SBThread thread = process->GetThreadAtIndex(thread_index);
  return thread.GetNumFrames();
}

std::string LLNodeApi::GetFrame(int thread_index, int frame_index) {
  lldb::SBThread thread = process->GetThreadAtIndex(thread_index);
  lldb::SBFrame frame = thread.GetFrameAtIndex(frame_index);
  lldb::SBSymbol symbol = frame.GetSymbol();

  std::string result;
  char buf[4096];
  if (symbol.IsValid()) {
    sprintf(buf, "Native: %s", frame.GetFunctionName());
    result += buf;

    lldb::SBModule module = frame.GetModule();
    lldb::SBFileSpec moduleFileSpec = module.GetFileSpec();
    sprintf(buf, " [%s/%s]", moduleFileSpec.GetDirectory(),
            moduleFileSpec.GetFilename());
    result += buf;

    lldb::SBCompileUnit compileUnit = frame.GetCompileUnit();
    lldb::SBFileSpec compileUnitFileSpec = compileUnit.GetFileSpec();
    if (compileUnitFileSpec.GetDirectory() != NULL ||
        compileUnitFileSpec.GetFilename() != NULL) {
      sprintf(buf, "\n\t [%s: %s]", compileUnitFileSpec.GetDirectory(),
              compileUnitFileSpec.GetFilename());
      result += buf;
    }
  } else {
    // V8 frame
    llnode::v8::Error err;
    llnode::v8::JSFrame v8_frame(v8_.get(),
                                 static_cast<int64_t>(frame.GetFP()));
    std::string frame_str = v8_frame.Inspect(true, err);

    // Skip invalid frames
    if (err.Fail() || frame_str.length() == 0 || frame_str[0] == '<') {
      if (frame_str[0] == '<') {
        sprintf(buf, "Unknown: %s", frame_str.c_str());
        result += buf;
      } else {
        result += "???";
      }
    } else {
      // V8 symbol
      sprintf(buf, "JavaScript: %s", frame_str.c_str());
      result += buf;
    }
  }
  return result;
}

int LLNodeApi::GetTypeCount() {
  lldb::SBCommandReturnObject result;
  /* Initial scan to create the JavaScript object map */
  if (!llscan->ScanHeapForObjects(*target, result)) {
    return 0;
  }

  // Load the object types into a vector
  TypeRecordMap::iterator end = llscan->GetMapsToInstances().end();
  object_types.clear();
  for (TypeRecordMap::iterator it = llscan->GetMapsToInstances().begin();
       it != end; ++it) {
    object_types.push_back(it->second);
  }

  // Sort by instance count
  std::sort(object_types.begin(), object_types.end(),
            TypeRecord::CompareInstanceCounts);
  return object_types.size();
}

std::string LLNodeApi::GetTypeName(int type_index) {
  TypeRecord* type = object_types.at(type_index);
  return type->GetTypeName();
}

int LLNodeApi::GetTypeInstanceCount(int type_index) {
  TypeRecord* type = object_types.at(type_index);
  return type->GetInstanceCount();
}

int LLNodeApi::GetTypeTotalSize(int type_index) {
  TypeRecord* type = object_types.at(type_index);
  return type->GetTotalInstanceSize();
}

std::set<uint64_t>& LLNodeApi::GetTypeInstances(int type_index) {
  TypeRecord* type = object_types.at(type_index);
  return type->GetInstances();
}

std::string LLNodeApi::GetObject(uint64_t address) {
  v8::Value v8_value(v8_.get(), address);
  v8::Value::InspectOptions inspect_options;
  inspect_options.detailed = true;
  inspect_options.length = 16;

  v8::Error err;
  std::string result = v8_value.Inspect(&inspect_options, err);
  if (err.Fail()) {
    return "Failed to get object";
  }
  return result;
}
}  // namespace llnode
