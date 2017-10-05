// C++ wrapper API for lldb and llnode APIs
#include <set>

namespace llnode {

int initSBTarget(char *filename, char *executable);
int getSBProcessInfo(int buffer_size, char *buffer);
int getSBProcessID();
char *getSBProcessState();
int getSBThreadCount();
int getSBFrameCount(int threadIndex);
int getSBFrame(int threadIndex, int frameIndex, int buffer_size, char *buffer);
int getSBTypeCount();
int getSBTypeName(int typeIndex, int buffer_size, char *buffer);
int getSBTypeInstanceCount(int typeIndex);
int getSBTypeTotalSize(int typeIndex);
std::set<uint64_t> &getSBTypeInstances(int typeIndex);
int getSBObject(uint64_t address, int buffer_size, char *buffer);

}  // namespace llnode
