#include <csignal>
#include <fstream>
#include <iostream>
#include <linenoise.h>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>

#include "breakpoint.h"
#include "debugger.h"
#include "dwarf/dwarf++.hh"
#include "registers.h"
void debugger::continue_execution() {
  ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
  wait_for_signal();
}

std::vector<std::string> split(const char *line, char delimiter = ' ') {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(line);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

void debugger::handle_command(const char *line) {
  auto args = split(line);
  auto cmd = args[0];
  if (cmd == "continue" || cmd == "c") {
    step_over_breakpoint();
    continue_execution();
  } else if (cmd == "break" || cmd == "b") {
    auto addr_str = args[1];
    std::intptr_t addr = std::stol(addr_str, 0, 16);
    set_breakpoint_at_address(addr);
  } else if (cmd == "register" || cmd == "regs") {
    if (args.size() == 1) {
      dump_registers();
    } else if (args.size() >= 3 && args[1] == "read") {
      std::cout << args[2] << " value : 0x" << std::hex
                << get_register_value(m_pid, string_to_reg(args[2]))
                << std::endl;
    } else if (args.size() >= 3 && args[1] == "write") {
      if (args.size() < 4) {
        std::cerr << "Usage: register write <reg_name> <value>\nValue must be "
                     "a valid hexadecimal number."
                  << std::endl;
        return;
      }
      std::string reg_name = args[2];
      uint64_t value =
          std::stoull(args[3], 0, 16); // Convert hex string to uint64_t
      set_register_value(m_pid, string_to_reg(reg_name), value);
      std::cout << "Set " << reg_name << " to 0x" << std::hex << value
                << std::endl;
    }
  } else if (cmd == "memory") {
    auto subcmd = args[1];
    if (subcmd == "read") {
      if (args.size() < 3) {
        std::cerr << "Usage: memory read <address>\nAddress must be a valid "
                     "hexadecimal number."
                  << std::endl;
        return;
      }
      std::uintptr_t address = std::stoull(args[2], 0, 16);
      std::cout << "Memory at 0x" << std::hex << address << ": 0x"
                << read_memory(address) << std::endl;
    } else if (subcmd == "write") {
      if (args.size() < 4) {
        std::cerr << "Usage: memory write <address> <value>\nAddress and value "
                     "must be valid hexadecimal numbers."
                  << std::endl;
        return;
      }
      std::uintptr_t address = std::stoull(args[2], 0, 16);
      uint64_t value = std::stoull(args[3], 0, 16);
      write_memory(address, value);
      std::cout << "Wrote 0x" << std::hex << value << " to memory at 0x"
                << address << std::endl;
    }
  } else {
    std::cerr << "Unknown command: " << line << std::endl;
  }
}

void debugger::run() {
  wait_for_signal();
  intialise_load_address();

  char *line = nullptr;
  while ((line = linenoise("4shdbg> ")) != nullptr) {
    handle_command(line);
    linenoiseFree(line);
    linenoiseHistoryAdd(line);
  }
}

void debugger::set_breakpoint_at_address(std::intptr_t addr) {
  std::cout << "Setting breakpoint at address 0x" << std::hex << addr
            << std::endl;
  breakpoint bp{m_pid, addr};
  bp.enable();
  m_breakpoints[addr] = bp;
}

void debugger::dump_registers() {
  for (const auto &desc : g_register_descriptors) {
    try {
      uint64_t value = get_register_value(m_pid, desc.r);
      std::cout << desc.name << " (dwarf reg " << desc.dwarf_r << "): 0x"
                << std::hex << value << std::dec << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error reading register " << desc.name << ": " << e.what()
                << std::endl;
    }
  }
}

uint64_t debugger::read_memory(uintptr_t address) {
  return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void debugger::write_memory(uintptr_t address, uint64_t value) {
  ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

uint64_t debugger::get_pc() { return get_register_value(m_pid, reg::rip); }

void debugger::set_pc(uint64_t pc) { set_register_value(m_pid, reg::rip, pc); }

void debugger::step_over_breakpoint() {
  auto possible_bp = get_pc();
  if (m_breakpoints.count(possible_bp)) {
    auto &bp = m_breakpoints[possible_bp];
    if (bp.is_enabled()) {
      bp.disable();
      ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
      wait_for_signal();
      bp.enable();
    }
  }
}

void debugger::wait_for_signal() {
  int wait_status;
  auto options = 0;
  waitpid(m_pid, &wait_status, options);

  siginfo_t siginfo = get_signal_info();

  switch (siginfo.si_signo) {
  case SIGTRAP:
    // handle trap signal (breakpoint hit or single step)
    handle_sigtrap(siginfo);
    break;
  case SIGSEGV:
    std::cout << "Your fav error: " << siginfo.si_code << std::endl;
    break;
  default:
    std::cout << "Received signal " << strsignal(siginfo.si_signo) << std::endl;
    break;
  }
}

dwarf::die debugger::get_function_from_pc(uint64_t pc) {
  for (auto &cu : m_dwarf.compilation_units()) {
    // Check if pc is in given CU
    if (dwarf::die_pc_range(cu.root()).contains(pc)) {
      // Iterate over all DIEs in CU to find the function DIE that contains the
      // PC
      for (const auto &die : cu.root()) {
        // We are only interested in subprogram DIEs, which represent functions
        if (die.tag == dwarf::DW_TAG::subprogram) {
          // Check if this function DIE's address range contains the PC
          if (dwarf::die_pc_range(die).contains(pc)) {
            return die;
          }
        }
      }
    }
  }

  throw std::runtime_error("Function not found for PC");
}

dwarf::line_table::iterator debugger::get_line_entry_from_pc(uint64_t pc) {
  for (const dwarf::compilation_unit &cu : m_dwarf.compilation_units()) {
    if (dwarf::die_pc_range(cu.root()).contains(pc)) {
      dwarf::line_table lt =
          cu.get_line_table(); // Get the line table for this compilation unit
      auto it = lt.find_address(
          pc); // Find the line entry corresponding to the given PC
      if (it != lt.end()) {
        return it;
      } else {
        throw std::out_of_range("Line entry not found for PC");
      }
    }
  }

  throw std::runtime_error("Cannot find line for given PC");
}

void debugger::intialise_load_address() {
  // If given binary has PIE enabled
  if (m_elf.get_hdr().type == elf::et::dyn) {
    // Base address can be found in /proc/<pid>/maps. The first entry is usually
    // the base address.

    std::ifstream maps_file("/proc/" + std::to_string(m_pid) + "/maps");
    if (!maps_file.is_open()) {
      throw std::runtime_error("Failed to open maps file");
    }

    std::string line;
    std::getline(maps_file, line, '-'); // Addr format: 00400000-0040b000 ....

    m_load_address = std::stoull(line, nullptr, 16);
  }
}

void debugger::print_src(const std::string &file_name, unsigned line,
                         unsigned n_lines) {
  std::ifstream file{file_name};
  if (!file.is_open()) {
    std::cerr << "Failed to open source file: " << file_name << std::endl;
    return;
  }

  // Print n_lines before and after the given line number
  unsigned start_line = (line > n_lines) ? line - n_lines : 1;
  unsigned end_line = line + n_lines +
                      (line <= n_lines ? n_lines - line + 1
                                       : 0); // Adjust end_line if line number
                                             // is less than or equal to n_lines

  char c{};
  unsigned current_line{1};

  // skip lines until start_line
  while (current_line < start_line && file.get(c)) {
    if (c == '\n')
      current_line++;
  }

  // output cursor
  std::cout << (current_line == line ? "-> " : "   ");

  // print lines from start_line to end_line
  while (current_line <= end_line && file.get(c)) {
    std::cout << c;
    if (c == '\n') {
      current_line++;
      if (current_line <= end_line) {
        std::cout << (current_line == line ? "-> " : "   ");
      }
    }
  }

  std::cout << std::endl;
}

siginfo_t debugger::get_signal_info() {
  siginfo_t siginfo;
  ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &siginfo);
  return siginfo;
}

void debugger::handle_sigtrap(siginfo_t info) {

  switch (info.si_code) {
  case SI_KERNEL:
    // std::cout << "Hit a breakpoint or single step (kernel)" << std::endl;

  case TRAP_BRKPT: {
    set_pc(get_pc() - 1);
    std::cout << "Hit a breakpoint at address 0x" << std::hex << get_pc()
              << std::endl;

    uint64_t offset_pc = offset_load_address(get_pc());
    auto line_entry_it = get_line_entry_from_pc(offset_pc);
    print_src(line_entry_it->file->path, line_entry_it->line, 5);

    return;
  }

  case TRAP_TRACE:
    // std::cout << "Single step completed" << std::endl;
    return;
  default:
    std::cout << "Received SIGTRAP with code " << info.si_code << std::endl;
    return;
  }
}