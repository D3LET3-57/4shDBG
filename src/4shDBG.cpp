#include "debugger.h"
#include <iostream>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <program-to-debug>" << std::endl;
        return 1;
    }

    auto prog = argv[1];
    int pid = fork();
    if (pid == 0)
    {
        // Child process: execute the program to be debugged
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
        execl(prog, prog, nullptr);
    }
    else if (pid > 0)
    {
        // Parent process: create debugger instance and run it
        std::cout<< "Starting debugging process " << pid << std::endl;
        debugger dbg{prog, pid};
        dbg.run();
    }
}