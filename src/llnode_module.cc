// Javascript module API for llnode/lldb
#include <stdlib.h>

#include <nan.h>
#include "src/llnode_api.h"

namespace llnode {

using Nan::FunctionCallbackInfo;
using v8::Array;
using v8::Context;
using v8::Exception;
using v8::Function;
using v8::FunctionTemplate;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Value;

NAN_METHOD(loadDump) {
  if (info.Length() < 2) {
    Nan::ThrowTypeError("Wrong number of args");
    return;
  }

  Nan::Utf8String filename(info[0]);
  Nan::Utf8String executable(info[1]);
  int ret = initSBTarget(*filename, *executable);
  if (ret == -1) {
    Nan::ThrowTypeError("Failed to load coredump");
  }
  info.GetReturnValue().Set(Nan::New("OK").ToLocalChecked());
}

NAN_METHOD(getProcessInfo) {
  char buffer[4096];

  getSBProcessInfo(4096, buffer);
  info.GetReturnValue().Set(Nan::New(buffer).ToLocalChecked());
}

// API version 2 - create JS object to introspect core dump
// process/threads/frames
NAN_METHOD(getProcessObject) {
  Nan::HandleScope scope;

  Local<Object> obj = Nan::New<Object>();
  obj->Set(Nan::New("pid").ToLocalChecked(), Nan::New(getSBProcessID()));
  obj->Set(Nan::New("state").ToLocalChecked(),
           Nan::New(getSBProcessState()).ToLocalChecked());
  obj->Set(Nan::New("threadCount").ToLocalChecked(),
           Nan::New(getSBThreadCount()));

  // Construct an array of objects to represent the threads for this process
  Local<Array> thread_list = Nan::New<Array>();
  for (int i = 0; i < getSBThreadCount(); i++) {
    Local<Object> thread = Nan::New<Object>();
    thread->Set(Nan::New("threadID").ToLocalChecked(), Nan::New(i));
    thread->Set(Nan::New("frameCount").ToLocalChecked(),
                Nan::New(getSBFrameCount(i)));
    // Construct an array of objects to represent the frames for this thread
    Local<Array> frame_list = Nan::New<Array>();
    for (int j = 0; j < getSBFrameCount(i); j++) {
      Local<Object> frame = Nan::New<Object>();
      char buffer[4096];
      getSBFrame(i, j, 4096, buffer);
      frame->Set(Nan::New("function").ToLocalChecked(),
                 Nan::New(buffer).ToLocalChecked());
      frame_list->Set(j, frame);
    }
    thread->Set(Nan::New("frames").ToLocalChecked(), frame_list);
    thread_list->Set(i, thread);
  }
  obj->Set(Nan::New("threads").ToLocalChecked(), thread_list);

  info.GetReturnValue().Set(obj);
}

// API version 2 - create JS object to introspect core dump heap/types/objects
std::set<uint64_t>::iterator* type_iterators;

NAN_METHOD(nextInstance) {
  Local<Object> type_obj = info.This();
  int typeIndex = type_obj->Get(Nan::New("index").ToLocalChecked())->IntegerValue();
  if (type_iterators[typeIndex] != getSBTypeInstances(typeIndex).end()) {
    Local<Object> obj = Nan::New<Object>();
    double addr = *(type_iterators[typeIndex]++);
    obj->Set(Nan::New("address").ToLocalChecked(), Nan::New(addr));
    info.GetReturnValue().Set(obj);
  }
}

NAN_METHOD(getHeapTypes) {
  Nan::HandleScope scope;

  // Construct an array of objects to represent the type maps for this heap
  Local<Array> type_list = Nan::New<Array>();
  // Allocate the type instance iterators
  type_iterators = (std::set<uint64_t>::iterator*)malloc(
      sizeof(std::set<uint64_t>::iterator) * getSBTypeCount());

  for (int i = 0; i < getSBTypeCount(); i++) {
    Local<Object> type = Nan::New<Object>();
    char buffer[4096];
    getSBTypeName(i, 4096, buffer);
    type->Set(Nan::New("index").ToLocalChecked(), Nan::New(i));
    type->Set(Nan::New("type").ToLocalChecked(),
              Nan::New(buffer).ToLocalChecked());
    type->Set(Nan::New("typeCount").ToLocalChecked(),
              Nan::New(getSBTypeInstanceCount(i)));
    type->Set(Nan::New("totalSize").ToLocalChecked(),
              Nan::New(getSBTypeTotalSize(i)));

    // Local<Array> instance_list = Nan::New<Array>();
    // int j = 0;
    // for (std::set<uint64_t>::iterator it = getSBTypeInstances(i).begin();
    //      it != getSBTypeInstances(i).end();
    //      ++it) {
    //  Local<Object> instance = Nan::New<Object>();
    //  instance->Set(Nan::New("address").ToLocalChecked(), Nan::New(*it));
    //  instance_list->Set(j++, instance);
    // }
    // Add array of instances to type object
    // type->Set(Nan::New("instances").ToLocalChecked(), instance_list);

    type_iterators[i] = getSBTypeInstances(i).begin();

    Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(nextInstance);
    Local<Function> fn = tpl->GetFunction();
    fn->SetName(Nan::New("nextInstance").ToLocalChecked());
    type->Set(Nan::New("nextInstance").ToLocalChecked(), fn);

    // Add type to array of types
    type_list->Set(i, type);
  }
  // obj->Set(Nan::New("types").ToLocalChecked();, type_list)

  info.GetReturnValue().Set(type_list);
}

NAN_METHOD(getObjectAtAddress) {
  Nan::HandleScope scope;
  Local<Object> obj = Nan::New<Object>();
  char buffer[4096];
  int64_t address = info[0]->IntegerValue();
  // fprintf(stderr,
  //         "llnode_module.cc GetObjectAtAddress() called for address %llx\n",
  //         address);

  getSBObject(address, 4096, buffer);

  obj->Set(Nan::New("address").ToLocalChecked(),
           Nan::New(static_cast<double>(address)));
  obj->Set(Nan::New("properties").ToLocalChecked(),
           Nan::New(buffer).ToLocalChecked());

  info.GetReturnValue().Set(obj);
}

NAN_METHOD(getThreadCount) {
  info.GetReturnValue().Set(Nan::New(getSBThreadCount()));
}

NAN_METHOD(getFrameCount) {
  int64_t index = info[0]->IntegerValue();
  info.GetReturnValue().Set(Nan::New(getSBFrameCount(index)));
}

NAN_METHOD(getFrame) {
  int64_t threadIndex = info[0]->IntegerValue();
  int64_t frameIndex = info[1]->IntegerValue();
  char buffer[4096];
  getSBFrame(threadIndex, frameIndex, 4096, buffer);
  info.GetReturnValue().Set(Nan::New(buffer).ToLocalChecked());
}

NAN_METHOD(getTypeCount) {
  info.GetReturnValue().Set(Nan::New(getSBTypeCount()));
}

NAN_METHOD(getTypeName) {
  int64_t typeIndex = info[0]->IntegerValue();
  char buffer[4096];
  getSBTypeName(typeIndex, 4096, buffer);
  info.GetReturnValue().Set(Nan::New(buffer).ToLocalChecked());
}

NAN_METHOD(getTypeInstanceCount) {
  int64_t typeIndex = info[0]->IntegerValue();
  info.GetReturnValue().Set(Nan::New(getSBTypeInstanceCount(typeIndex)));
}

NAN_METHOD(getTypeTotalSize) {
  int64_t typeIndex = info[0]->IntegerValue();
  info.GetReturnValue().Set(Nan::New(getSBTypeTotalSize(typeIndex)));
}

NAN_MODULE_INIT(Init) {
  NAN_EXPORT(target, loadDump);
  NAN_EXPORT(target, getProcessInfo);
  NAN_EXPORT(target, getProcessObject);
  NAN_EXPORT(target, getHeapTypes);
  NAN_EXPORT(target, getThreadCount);
  NAN_EXPORT(target, getFrameCount);
  NAN_EXPORT(target, getFrame);
  NAN_EXPORT(target, getTypeCount);
  NAN_EXPORT(target, getTypeName);
  NAN_EXPORT(target, getTypeInstanceCount);
  NAN_EXPORT(target, getTypeTotalSize);
  NAN_EXPORT(target, getObjectAtAddress);
}

NODE_MODULE(llnode_module, Init)

}  // namespace llnode
