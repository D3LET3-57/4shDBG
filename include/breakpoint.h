#ifndef BREAKPOINT_H
#define BREAKPOINT_H
#include <cstdint>
#include <sys/types.h>

class breakpoint
{
private:
    pid_t m_pid;
    std::intptr_t m_addr;
    bool m_enabled;
    uint8_t m_saved_data;

public:
    breakpoint() : m_pid{0}, m_addr{0}, m_enabled{false}, m_saved_data{0} {}
    breakpoint(pid_t pid, std::intptr_t addr)
        : m_pid{pid}, m_addr{addr}, m_enabled{false}, m_saved_data{0} {}

    void enable();
    void disable();
    bool is_enabled() const { return m_enabled; };
    std::intptr_t get_address() const { return m_addr; };
};

#endif // BREAKPOINT_H