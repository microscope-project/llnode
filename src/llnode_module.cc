// Javascript module API for llnode/lldb
#include <stdlib.h>

#include "src/llnode_api.h"
#include "src/llnode_module.h"

namespace llnode {

using ::v8::Array;
using ::v8::Context;
using ::v8::Exception;
using ::v8::Function;
using ::v8::FunctionTemplate;
using ::v8::HandleScope;
using ::v8::Isolate;
using ::v8::Local;
using ::v8::Number;
using ::v8::Object;
using ::v8::String;
using ::v8::Value;
using Nan::FunctionCallbackInfo;
using Nan::New;

NAN_MODULE_INIT(LLNode::Init) {
  // Prepare constructor template
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("LLNode").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  // Prototype
  Nan::SetPrototypeMethod(tpl, "getProcessInfo", GetProcessInfo);
  Nan::SetPrototypeMethod(tpl, "getProcessObject", GetProcessObject);
  Nan::SetPrototypeMethod(tpl, "getHeapTypes", GetHeapTypes);
  Nan::SetPrototypeMethod(tpl, "getThreadCount", GetThreadCount);
  Nan::SetPrototypeMethod(tpl, "getFrameCount", GetFrameCount);
  Nan::SetPrototypeMethod(tpl, "getFrame", GetFrame);
  Nan::SetPrototypeMethod(tpl, "getTypeCount", GetTypeCount);
  Nan::SetPrototypeMethod(tpl, "getTypeName", GetTypeName);
  Nan::SetPrototypeMethod(tpl, "getTypeInstanceCount", GetTypeInstanceCount);
  Nan::SetPrototypeMethod(tpl, "getTypeTotalSize", GetTypeTotalSize);
  Nan::SetPrototypeMethod(tpl, "getObjectAtAddress", GetObjectAtAddress);

  // Set up constructor
  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  // Export factory methods to module.exports
  Nan::Set(target, Nan::New("fromCoredump").ToLocalChecked(),
           Nan::New<FunctionTemplate>(FromCoreDump)->GetFunction());
  // Nan::Set(target, Nan::New("fromPid").ToLocalChecked(),
  //          Nan::New<FunctionTemplate>(FromPid)->GetFunction());
}

LLNode::LLNode() : llnode_ptr(new llnode::LLNodeApi()) {}

LLNode::~LLNode() {}

NAN_METHOD(LLNode::New) {
  if (info.IsConstructCall()) {
    if (info.Length() == 2) {  // (executable, corename)
      Nan::Utf8String filename(info[0]);
      Nan::Utf8String executable(info[1]);

      LLNode* obj = new LLNode();
      int ret = obj->llnode_ptr->Init(*filename, *executable);
      if (ret == -1) {
        Nan::ThrowTypeError("Failed to load coredump");
        return;
      }
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
      return;
      // pid
      // } else if (info.Length() == 1) {
      //   int pid = Nan::To<int>(info[0]).FromJust();
    } else {
      Nan::ThrowTypeError("Wrong number of args");
      return;
    }
  } else {  // called without new
    Local<Function> cons = Nan::New(constructor());
    int argc = info.Length();
    Local<Value> argv[argc];
    for (int i = 0; i < argc; ++i) {
      argv[i] = info[i];
    }
    info.GetReturnValue().Set(
        Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(LLNode::FromCoreDump) {
  if (info.Length() < 2) {
    Nan::ThrowTypeError("Wrong number of args");
    return;
  }
  int argc = info.Length();
  Local<Value> argv[argc];
  for (int i = 0; i < argc; ++i) {
    argv[i] = info[i];
  }
  Local<Function> cons = Nan::New(constructor());
  info.GetReturnValue().Set(
      Nan::NewInstance(cons, argc, argv).ToLocalChecked());
}

NAN_METHOD(LLNode::GetProcessInfo) {
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  std::string process_info = obj->llnode_ptr->GetProcessInfo();
  info.GetReturnValue().Set(Nan::New(process_info).ToLocalChecked());
}

// API version 2 - create JS object to introspect core dump
// process/threads/frames
NAN_METHOD(LLNode::GetProcessObject) {
  Nan::HandleScope scope;
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int pid = obj->llnode_ptr->GetProcessID();
  std::string state = obj->llnode_ptr->GetProcessState();
  int thread_count = obj->llnode_ptr->GetThreadCount();

  Local<Object> result = Nan::New<Object>();
  result->Set(Nan::New("pid").ToLocalChecked(), Nan::New(pid));
  result->Set(Nan::New("state").ToLocalChecked(),
              Nan::New(state).ToLocalChecked());
  result->Set(Nan::New("threadCount").ToLocalChecked(), Nan::New(thread_count));

  // Construct an array of objects to represent the threads for this process
  Local<Array> thread_list = Nan::New<Array>();
  for (int i = 0; i < thread_count; i++) {
    Local<Object> thread = Nan::New<Object>();
    thread->Set(Nan::New("threadID").ToLocalChecked(), Nan::New(i));
    int frame_count = obj->llnode_ptr->GetFrameCount(i);
    thread->Set(Nan::New("frameCount").ToLocalChecked(), Nan::New(frame_count));
    // Construct an array of objects to represent the frames for this thread
    Local<Array> frame_list = Nan::New<Array>();
    for (int j = 0; j < frame_count; j++) {
      Local<Object> frame = Nan::New<Object>();
      std::string frame_str = obj->llnode_ptr->GetFrame(i, j);
      frame->Set(Nan::New("function").ToLocalChecked(),
                 Nan::New(frame_str).ToLocalChecked());
      frame_list->Set(j, frame);
    }
    thread->Set(Nan::New("frames").ToLocalChecked(), frame_list);
    thread_list->Set(i, thread);
  }
  result->Set(Nan::New("threads").ToLocalChecked(), thread_list);

  info.GetReturnValue().Set(result);
}

NAN_METHOD(LLNode::NextInstance) {
  Local<Object> type_obj = info.This();
  // Todo: context
  int type_index = type_obj->Get(Nan::New("index").ToLocalChecked())
                       ->IntegerValue(Nan::GetCurrentContext())
                       .FromJust();
  Local<Object> llnode_obj = type_obj->Get(Nan::New("llnode").ToLocalChecked())
                                 ->ToObject(Nan::GetCurrentContext())
                                 .ToLocalChecked();

  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(llnode_obj);
  std::set<uint64_t>& type_instances =
      obj->llnode_ptr->GetTypeInstances(type_index);
  TypeIteratorVec type_iterators = obj->type_iterators;
  if (type_iterators[type_index] != type_instances.end()) {
    Local<Object> result = Nan::New<Object>();
    double addr = static_cast<double>(*(type_iterators[type_index]++));
    result->Set(Nan::New("address").ToLocalChecked(), Nan::New(addr));
    info.GetReturnValue().Set(result);
  }
}

NAN_METHOD(LLNode::GetHeapTypes) {
  Nan::HandleScope scope;

  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  // Construct an array of objects to represent the type maps for this heap
  Local<Array> type_list = Nan::New<Array>();
  int type_count = obj->llnode_ptr->GetTypeCount();
  // Allocate the type instance iterators
  obj->type_iterators = TypeIteratorVec(type_count);

  for (int i = 0; i < type_count; i++) {
    Local<Object> type = Nan::New<Object>();
    std::string type_name = obj->llnode_ptr->GetTypeName(i);
    int type_ins_count = obj->llnode_ptr->GetTypeInstanceCount(i);
    int type_total_size = obj->llnode_ptr->GetTypeTotalSize(i);
    type->Set(Nan::New("index").ToLocalChecked(), Nan::New(i));
    type->Set(Nan::New("type").ToLocalChecked(),
              Nan::New(type_name).ToLocalChecked());
    type->Set(Nan::New("typeCount").ToLocalChecked(), Nan::New(type_ins_count));
    type->Set(Nan::New("totalSize").ToLocalChecked(),
              Nan::New(type_total_size));

    type->Set(Nan::New("llnode").ToLocalChecked(), info.Holder());

    std::set<uint64_t>& type_instances = obj->llnode_ptr->GetTypeInstances(i);
    TypeIteratorVec& type_iterators = obj->type_iterators;
    type_iterators[i] = type_instances.begin();
    // Local<Array> instance_list = Nan::New<Array>();
    // int j = 0;
    // for (std::set<uint64_t>::iterator it = type_iterators[i];
    //      it != getSBTypeInstances(i).end();
    //      ++it) {
    //  Local<Object> instance = Nan::New<Object>();
    //  instance->Set(Nan::New("address").ToLocalChecked(), Nan::New(*it));
    //  instance_list->Set(j++, instance);
    // }
    // Add array of instances to type object
    // type->Set(Nan::New("instances").ToLocalChecked(), instance_list);

    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(NextInstance);
    Local<Function> fn = tpl->GetFunction();
    fn->SetName(Nan::New("nextInstance").ToLocalChecked());
    type->Set(Nan::New("nextInstance").ToLocalChecked(), fn);

    // Add type to array of types
    type_list->Set(i, type);
  }
  // obj->Set(Nan::New("types").ToLocalChecked();, type_list)

  info.GetReturnValue().Set(type_list);
}

NAN_METHOD(LLNode::GetObjectAtAddress) {
  Nan::HandleScope scope;
  Local<Object> result = Nan::New<Object>();
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int64_t address = info[0]->IntegerValue();  // Todo: use the maybe version
  std::string properties = obj->llnode_ptr->GetObject(address);

  result->Set(Nan::New("address").ToLocalChecked(),
              Nan::New(static_cast<double>(address)));
  result->Set(Nan::New("properties").ToLocalChecked(),
              Nan::New(properties).ToLocalChecked());

  info.GetReturnValue().Set(result);
}

NAN_METHOD(LLNode::GetThreadCount) {
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int thread_count = obj->llnode_ptr->GetThreadCount();
  info.GetReturnValue().Set(Nan::New(thread_count));
}

NAN_METHOD(LLNode::GetFrameCount) {
  int64_t index = info[0]->IntegerValue();
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int frame_count = obj->llnode_ptr->GetFrameCount(index);
  info.GetReturnValue().Set(Nan::New(frame_count));
}

NAN_METHOD(LLNode::GetFrame) {
  int64_t thread_index = info[0]->IntegerValue();
  int64_t frame_index = info[1]->IntegerValue();
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  std::string frame_str = obj->llnode_ptr->GetFrame(thread_index, frame_index);
  info.GetReturnValue().Set(Nan::New(frame_str).ToLocalChecked());
}

NAN_METHOD(LLNode::GetTypeCount) {
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int type_count = obj->llnode_ptr->GetTypeCount();
  info.GetReturnValue().Set(Nan::New(type_count));
}

NAN_METHOD(LLNode::GetTypeName) {
  int64_t type_index = info[0]->IntegerValue();
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  std::string type_name = obj->llnode_ptr->GetTypeName(type_index);
  info.GetReturnValue().Set(Nan::New(type_name).ToLocalChecked());
}

NAN_METHOD(LLNode::GetTypeInstanceCount) {
  int64_t type_index = info[0]->IntegerValue();
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int type_ins_count = obj->llnode_ptr->GetTypeInstanceCount(type_index);
  info.GetReturnValue().Set(Nan::New(type_ins_count));
}

NAN_METHOD(LLNode::GetTypeTotalSize) {
  int64_t type_index = info[0]->IntegerValue();
  LLNode* obj = Nan::ObjectWrap::Unwrap<LLNode>(info.Holder());
  int type_total_size = obj->llnode_ptr->GetTypeTotalSize(type_index);
  info.GetReturnValue().Set(Nan::New(type_total_size));
}

}  // namespace llnode
