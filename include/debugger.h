#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include "breakpoint.h"

class debugger
{
private:
    std::string m_prog_name;
    int pid;
    std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;

public:
    debugger(const std::string &prog_name, int pid)
        : m_prog_name{prog_name}, pid{pid} {}
    void run();
    void set_breakpoint_at_address(std::intptr_t addr);

private:
    void continue_execution();
    void handle_command(const char *line);
};
#endif // DEBUGGER_H