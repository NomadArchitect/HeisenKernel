#include <api.h>

#include <syscall.h>
#include <log.h>
#include <proc.h>

using namespace LIBHeisenKernel;

void API::Initialize()
{
    //Call kernel to set this process as a cactusos process
    Process::ID = DoSyscall(SYSCALL_SET_CACTUSOS_LIB);

    Log(Info, "CactusOS API Initialized for this process");
}