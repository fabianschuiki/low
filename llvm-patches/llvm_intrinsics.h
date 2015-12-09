

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif	

typedef unsigned int LLVMIntrinsicID;
extern const LLVMIntrinsicID LLVMIntrinsicIDTrap;
LLVMValueRef LLVMGetIntrinsicByID(LLVMModuleRef mod,LLVMIntrinsicID id, LLVMTypeRef* tys,int nty);

#ifdef __cplusplus
}
#endif