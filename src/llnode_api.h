// C++ wrapper API for lldb and llnode APIs
#include <set>
#include <string>
#include <vector>
#include <memory>
#ifndef SRC_LLNODE_API_H_
#define SRC_LLNODE_API_H_

namespace lldb {
class SBDebugger;
class SBTarget;
class SBProcess;

}  // namespace lldb

namespace llnode {

class LLScan;
class TypeRecord;

namespace v8 {
class LLV8;
}


class LLNodeApi {
 public:
  LLNodeApi();
  ~LLNodeApi();
  LLNodeApi(LLNodeApi &&);
  LLNodeApi &operator=(LLNodeApi &&);

  int Init(const char *filename, const char *executable);
  std::string GetProcessInfo();
  int GetProcessID();
  std::string GetProcessState();
  int GetThreadCount();
  int GetFrameCount(int thread_index);
  std::string GetFrame(int thread_index, int frame_index);
  int GetTypeCount();
  std::string GetTypeName(int type_index);
  int GetTypeInstanceCount(int type_index);
  int GetTypeTotalSize(int type_index);
  std::set<uint64_t> &GetTypeInstances(int type_index);
  std::string GetObject(uint64_t address);

 private:
  bool loaded;
  std::unique_ptr<lldb::SBDebugger> debugger;
  std::unique_ptr<lldb::SBTarget> target;
  std::unique_ptr<lldb::SBProcess> process;
  std::unique_ptr<LLScan> llscan;
  std::vector<TypeRecord *> object_types;
  std::unique_ptr<v8::LLV8> v8_;
};

}  // namespace llnode

#endif  // SRC_LLNODE_API_H_
