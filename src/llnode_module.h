#ifndef SRC_LLNODE_MODULE_H
#define SRC_LLNODE_MODULE_H

#include <nan.h>
#include <set>
#include <vector>

namespace llnode {
class LLNodeApi;

class LLNode : public Nan::ObjectWrap {
 public:
  static NAN_MODULE_INIT(Init);

 private:
  explicit LLNode();
  ~LLNode();

  static NAN_METHOD(New);
  static NAN_METHOD(FromCoreDump);
  // static NAN_METHOD(FromPid);
  // static NAN_METHOD(LoadDump);
  static NAN_METHOD(GetProcessInfo);
  static NAN_METHOD(GetProcessObject);
  static NAN_METHOD(GetHeapTypes);
  static NAN_METHOD(NextInstance);
  static NAN_METHOD(GetThreadCount);
  static NAN_METHOD(GetFrameCount);
  static NAN_METHOD(GetFrame);
  static NAN_METHOD(GetTypeCount);
  static NAN_METHOD(GetTypeName);
  static NAN_METHOD(GetTypeInstanceCount);
  static NAN_METHOD(GetTypeTotalSize);
  static NAN_METHOD(GetObjectAtAddress);

  static inline Nan::Persistent<::v8::Function>& constructor() {
    static Nan::Persistent<::v8::Function> my_constructor;
    return my_constructor;
  }

  std::unique_ptr<llnode::LLNodeApi> llnode_ptr;

  typedef std::vector<std::set<uint64_t>::iterator> TypeIteratorVec;
  TypeIteratorVec type_iterators;
};

}  // namespace llnode

#endif
