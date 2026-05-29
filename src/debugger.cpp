#include "debugger.h"
#include <iostream>
#include <stdexcept>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linenoise.h>
#include "breakpoint.h"
#include "dwarf/dwarf++.hh"
#include "registers.h"
#include <vector>
#include <sstream>
#include "registers.h"
void debugger::continue_execution()
{
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);

    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

std::vector<std::string> split(const char* line, char delimiter = ' ')
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

void debugger::handle_command(const char *line)
{   
    auto args = split(line);
    auto cmd = args[0];
    if (cmd == "continue" || cmd == "c")
    {
        step_over_breakpoint();
        continue_execution();
    }
    else if (cmd == "break" || cmd == "b")
    {
        auto addr_str = args[1];
        std::intptr_t addr = std::stol(addr_str, 0, 16);
        set_breakpoint_at_address(addr);
    }
    else if (cmd == "register" || cmd == "regs")
    {
        if(args.size() == 1) {
            dump_registers();
        }
        else if(args.size() >= 3 && args[1] == "read") {
            std::cout<< args[2] << " value : 0x" << std::hex << get_register_value(m_pid, string_to_reg(args[2])) << std::endl;
        }
        else if(args.size() >= 3 && args[1] == "write") {
            if(args.size() < 4) {
                std::cerr << "Usage: register write <reg_name> <value>\nValue must be a valid hexadecimal number." << std::endl;
                return;
            }
            std::string reg_name = args[2]; 
            uint64_t value = std::stoull(args[3], 0, 16); // Convert hex string to uint64_t
            set_register_value(m_pid, string_to_reg(reg_name), value);
            std::cout<< "Set " << reg_name << " to 0x" << std::hex << value << std::endl;
        }
    }
    else if (cmd == "memory"){
        auto subcmd = args[1];
        if(subcmd == "read"){
            if(args.size() < 3) {
                std::cerr << "Usage: memory read <address>\nAddress must be a valid hexadecimal number." << std::endl;
                return;
            }
            std::uintptr_t address = std::stoull(args[2], 0, 16);
            std::cout << "Memory at 0x" << std::hex << address << ": 0x" << read_memory(address) << std::endl;
        }
        else if(subcmd=="write"){
            if(args.size() < 4) {
                std::cerr << "Usage: memory write <address> <value>\nAddress and value must be valid hexadecimal numbers." << std::endl;
                return;
            }
            std::uintptr_t address = std::stoull(args[2], 0, 16);
            uint64_t value = std::stoull(args[3], 0, 16);
            write_memory(address, value);
            std::cout << "Wrote 0x" << std::hex << value << " to memory at 0x" << address << std::endl;
        }
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
    waitpid(m_pid, &wait_status, options);

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
    breakpoint bp{m_pid, addr};
    bp.enable();
    m_breakpoints[addr] = bp;
}

void debugger::dump_registers()
{
    for (const auto& desc : g_register_descriptors) {
        try {
            uint64_t value = get_register_value(m_pid, desc.r);
            std::cout << desc.name << " (dwarf reg " << desc.dwarf_r << "): 0x" << std::hex << value << std::dec << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error reading register " << desc.name << ": " << e.what() << std::endl;
        }
    }
}

uint64_t debugger::read_memory(uintptr_t address)
{
    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(uintptr_t address, uint64_t value)
{
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

uint64_t debugger::get_pc()
{
    return get_register_value(m_pid, reg::rip);
}

void debugger::set_pc(uint64_t pc)
{
    set_register_value(m_pid, reg::rip, pc);
}

void debugger::step_over_breakpoint()
{
    auto possible_bp = get_pc() - 1;
    // -1 because the breakpoint instruction (int3) is 1 byte long, so the PC will be right after it when we hit the breakpoint
    if(m_breakpoints.count(possible_bp)){
        auto& bp = m_breakpoints[possible_bp];
        if(bp.is_enabled()){
            // To continue the execution after hitting a breakpoint, we need to:
            // Move the PC back to original addr -> Disable the breakpoint -> Single step -> disable the bp -> Enable the bp.
            auto prev_pc = possible_bp; 
            set_pc(prev_pc);

            bp.disable();
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
            wait_for_signal();
            bp.enable();
        }
    }
}

void debugger::wait_for_signal()
{
    int wait_status;
    auto options = 0;
    waitpid(m_pid, &wait_status, options);
}

dwarf::die debugger::get_function_from_pc(uint64_t pc)
{
    for(auto &cu : m_dwarf.compilation_units()){
        // Check if pc is in given CU
        if(dwarf::die_pc_range(cu.root()).contains(pc)){
            // Iterate over all DIEs in CU to find the function DIE that contains the PC
            for(const auto& die : cu.root()){
                // We are only interested in subprogram DIEs, which represent functions
                if(die.tag == dwarf::DW_TAG::subprogram){
                    // Check if this function DIE's address range contains the PC
                    if(dwarf::die_pc_range(die).contains(pc)){
                        return die;
                    }
                }
            }
        }
    }
    
    throw std::runtime_error("Function not found for PC");
}

dwarf::line_table::iterator debugger::get_line_entry_from_pc(uint64_t pc)
{
    for(const dwarf::compilation_unit& cu : m_dwarf.compilation_units()){
        if(dwarf::die_pc_range(cu.root()).contains(pc)){
            dwarf::line_table lt = cu.get_line_table(); // Get the line table for this compilation unit
            auto it = lt.find_address(pc); // Find the line entry corresponding to the given PC
            if(it != lt.end()){
                return it;
            }
            else{
                throw std::out_of_range("Line entry not found for PC");
            }
        }
    }

    throw std::runtime_error("Cannot find line for given PC");

}