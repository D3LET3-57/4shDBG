#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <string>

class debugger
{
private:
    std::string m_prog_name;
    int pid;

public:
    debugger(const std::string &prog_name, int pid)
        : m_prog_name{prog_name}, pid{pid} {}
    void run();

private:
    void continue_execution();
    void handle_command(const char *line);
};
#endif // DEBUGGER_H