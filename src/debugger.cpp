#include "debugger.h"
#include <iostream>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linenoise.h>
#include "breakpoint.h"
void debugger::continue_execution()
{
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);

    int wait_status;
    auto options = 0;
    waitpid(pid, &wait_status, options);
}

void debugger::handle_command(const char *line)
{
    if (std::string(line) == "continue" || std::string(line) == "c")
    {
        continue_execution();
    }
    else if (std::string(line).substr(0, 6) == "break ")
    {
        auto addr_str = std::string(line).substr(6);
        std::intptr_t addr = std::stol(addr_str, 0, 16);
        set_breakpoint_at_address(addr);
    }
    else
    {
        std::cerr << "Unknown command: " << line << std::endl;
    }
}

void debugger::run()
{
    int wait_status;
    auto options = 0;
    waitpid(pid, &wait_status, options);

    char *line = nullptr;
    while ((line = linenoise("4shdbg> ")) != nullptr)
    {
        handle_command(line);
        linenoiseFree(line);
        linenoiseHistoryAdd(line);
    }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr)
{
    std::cout << "Setting breakpoint at address 0x" << std::hex << addr << std::endl;
    breakpoint bp{pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}