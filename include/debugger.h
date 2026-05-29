#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include <fcntl.h>

#include "breakpoint.h"
#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

class debugger
{
private:
    std::string m_prog_name;
    int m_pid;
    std::unordered_map<std::intptr_t, breakpoint> m_breakpoints;
    dwarf::dwarf m_dwarf;
    elf::elf m_elf;

public:
    debugger(const std::string &prog_name, int pid)
        : m_prog_name{prog_name}, m_pid{pid} {
            auto fd = open(m_prog_name.c_str(), O_RDONLY);
            // open is used instead of std::ifstream because elf loader needs a fd to pass it to mmap so that it can map the file into memory rather than reading it into memory all at once.
            m_elf = elf::elf{elf::create_mmap_loader(fd)};
            m_dwarf = dwarf::dwarf{dwarf::elf::create_loader(m_elf)};
        } // Constructor
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
    dwarf::die get_function_from_pc(uint64_t pc);
    dwarf::line_table::iterator get_line_entry_from_pc(uint64_t pc);

private:
    void continue_execution();
    void handle_command(const char *line);
};
#endif // DEBUGGER_H