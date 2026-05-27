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
    int m_pid;
    std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;

public:
    debugger(const std::string &prog_name, int pid)
        : m_prog_name{prog_name}, m_pid{pid} {}
    void run();
    void set_breakpoint_at_address(std::intptr_t addr);
    void dump_registers();
    uint64_t read_memory(uintptr_t address);
    void write_memory(uintptr_t address, uint64_t value);
    // Helper functions
    uint64_t get_pc();
    void set_pc(uint64_t pc);
    void step_over_breakpoint();
    void wait_for_signal();

private:
    void continue_execution();
    void handle_command(const char *line);
};
#endif // DEBUGGER_H