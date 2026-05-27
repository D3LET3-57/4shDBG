#ifndef REGISTER_H
#define REGISTER_H


#include <stdexcept>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <array>
#include <string>
#include <cstdint>
#include <algorithm>

enum class reg {
    rax, rbx, rcx, rdx,
    rdi, rsi, rbp, rsp,
    r8,  r9,  r10, r11,
    r12, r13, r14, r15,
    rip, rflags,    cs,
    orig_rax, fs_base,
    gs_base,
    fs, gs, ss, ds, es
};

constexpr std::size_t n_registers = 27;

struct reg_descriptor {
    reg r;
    int dwarf_r;
    std::string name;
};

static const std::array<reg_descriptor, n_registers> g_register_descriptors {{
        { reg::r15, 15, "r15" },
        { reg::r14, 14, "r14" },
        { reg::r13, 13, "r13" },
        { reg::r12, 12, "r12" },
        { reg::rbp, 6, "rbp" },
        { reg::rbx, 3, "rbx" },
        { reg::r11, 11, "r11" },
        { reg::r10, 10, "r10" },
        { reg::r9, 9, "r9" },
        { reg::r8, 8, "r8" },
        { reg::rax, 0, "rax" },
        { reg::rcx, 2, "rcx" },
        { reg::rdx, 1, "rdx" },
        { reg::rsi, 4, "rsi" },
        { reg::rdi, 5, "rdi" },
        { reg::orig_rax, -1, "orig_rax" },
        { reg::rip, -1, "rip" },
        { reg::cs, 51, "cs" },
        { reg::rflags, 49, "eflags" },
        { reg::rsp, 7, "rsp" },
        { reg::ss, 52, "ss" },
        { reg::fs_base, 58, "fs_base" },
        { reg::gs_base, 59, "gs_base" },
        { reg::ds, 53, "ds" },
        { reg::es, 50, "es" },
        { reg::fs, 54, "fs" },
        { reg::gs, 55, "gs" },
}};

inline uint64_t get_register_value(pid_t pid, reg r){
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    auto desc_it = std::find_if(g_register_descriptors.begin(), g_register_descriptors.end(),
                                [r](const reg_descriptor& desc) { return desc.r == r; });
    
    if (desc_it != g_register_descriptors.end()) {
        const auto& desc = *desc_it;
        switch (desc.r) {
            case reg::rax: return regs.rax;
            case reg::rbx: return regs.rbx;
            case reg::rcx: return regs.rcx;
            case reg::rdx: return regs.rdx;
            case reg::rdi: return regs.rdi;
            case reg::rsi: return regs.rsi;
            case reg::rbp: return regs.rbp;
            case reg::rsp: return regs.rsp;
            case reg::r8:  return regs.r8;
            case reg::r9:  return regs.r9;
            case reg::r10: return regs.r10;
            case reg::r11: return regs.r11;
            case reg::r12: return regs.r12;
            case reg::r13: return regs.r13;
            case reg::r14: return regs.r14;
            case reg::r15: return regs.r15;
            case reg::rip: return regs.rip;
            case reg::rflags: return regs.eflags;
            case reg::cs: return regs.cs;
            case reg::orig_rax: return regs.orig_rax;
            case reg::fs_base: return regs.fs_base;
            case reg::gs_base: return regs.gs_base;
            case reg::fs: return regs.fs;
            case reg::gs: return regs.gs;
            case reg::ss: return regs.ss;
            case reg::ds: return regs.ds;
            default: throw std::invalid_argument("Unknown register"); // Should not happen
        }
    }
    throw std::out_of_range("Unknown register && register number: " + std::to_string(static_cast<int>(r)));
    return 0; // Register not found
}

inline void set_register_value(pid_t pid, reg r, uint64_t value){
    user_regs_struct regs;
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);

    auto desc_it = std::find_if(g_register_descriptors.begin(), g_register_descriptors.end(),
                                [r](const reg_descriptor& desc) { return desc.r == r; });
    if (desc_it != g_register_descriptors.end()) {
        const auto& desc = *desc_it;
        switch (desc.r) {
            case reg::rax: regs.rax = value; break;
            case reg::rbx: regs.rbx = value; break;
            case reg::rcx: regs.rcx = value; break;
            case reg::rdx: regs.rdx = value; break;
            case reg::rdi: regs.rdi = value; break;
            case reg::rsi: regs.rsi = value; break;
            case reg::rbp: regs.rbp = value; break;
            case reg::rsp: regs.rsp = value; break;
            case reg::r8:  regs.r8 = value; break;
            case reg::r9:  regs.r9 = value; break;
            case reg::r10: regs.r10 = value; break;
            case reg::r11: regs.r11 = value; break;
            case reg::r12: regs.r12 = value; break;
            case reg::r13: regs.r13 = value; break;
            case reg::r14: regs.r14 = value; break;
            case reg::r15: regs.r15 = value; break;
            case reg::rip: regs.rip = value; break;
            case reg::rflags: regs.eflags = value; break;
            case reg::cs: regs.cs = value; break;
            case reg::orig_rax: regs.orig_rax = value; break;
            case reg::fs_base: regs.fs_base = value; break;
            case reg::gs_base: regs.gs_base = value; break;
            case reg::fs: regs.fs = value; break;   
            case reg::gs: regs.gs = value; break;
            case reg::ss: regs.ss = value; break;
            case reg::ds: regs.ds = value; break;
            default: throw std::invalid_argument("Unknown register && register number: " + std::to_string(static_cast<int>(r))); // Should not happen
        }
    }

    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
}

inline uint64_t get_register_value_from_dwarf(pid_t pid, unsigned regNum){
    auto desc_it = std::find_if(g_register_descriptors.begin(), g_register_descriptors.end(),
                                [regNum](const reg_descriptor& desc) { return desc.dwarf_r == static_cast<int>(regNum); });
    
    if (desc_it != g_register_descriptors.end()) {
        return get_register_value(pid, desc_it->r);
    }
    throw std::out_of_range("Unknown DWARF register number: " + std::to_string(regNum));
    return 0; // Register not found
}

inline std::string reg_to_string(reg r) {
    auto desc_it = std::find_if(g_register_descriptors.begin(), g_register_descriptors.end(),
                                [r](const reg_descriptor& desc) { return desc.r == r; });
    if (desc_it != g_register_descriptors.end()) {
        return desc_it->name;
    }
    throw std::out_of_range("Unknown register: " + std::to_string(static_cast<int>(r)));
    return "unknown";
}

inline reg string_to_reg(const std::string& name) {
    auto desc_it = std::find_if(g_register_descriptors.begin(), g_register_descriptors.end(),
                                [&name](const reg_descriptor& desc) { return desc.name == name; });
    if (desc_it != g_register_descriptors.end()) {
        return desc_it->r;
    }
    throw std::out_of_range("Unknown register name: " + name);
    return reg::rax; // Default return to avoid compiler warning, should not reach here
}

#endif // REGISTER_H