#include "breakpoint.h"
#include <sys/ptrace.h>

void breakpoint::enable()
{
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    m_saved_data = static_cast<uint8_t>(data & 0xFF);
    uint64_t int3 = 0xcc;
    uint64_t data_with_int3 = (data & ~0xFF) | int3;
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, data_with_int3);
    m_enabled = true;
}

void breakpoint::disable()
{
    auto data = ptrace(PTRACE_PEEKDATA, m_pid, m_addr, nullptr);
    uint64_t restored_data = (data & ~0xFF) | m_saved_data;
    ptrace(PTRACE_POKEDATA, m_pid, m_addr, restored_data);
    m_enabled = false;
}