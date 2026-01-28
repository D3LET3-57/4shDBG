#include "debugger.h"
#include <iostream>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linenoise.h>

void debugger::continue_execution(){
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    
    int wait_status;
    auto options = 0;
    waitpid(pid, &wait_status, options);
}

void debugger::handle_command(const char* line){
    if(std::string(line) == "continue" || std::string(line) == "c"){
        continue_execution();
    } else {
        std::cerr << "Unknown command: " << line << std::endl;
    }
}

void debugger::run(){
    int wait_status;
    auto options = 0;
    waitpid(pid, &wait_status, options);

    char *line = nullptr;
    while((line = linenoise("4shdbg> "))!=nullptr){
        handle_command(line);
        linenoiseFree(line);
        linenoiseHistoryAdd(line);
    }
}


int main(int argc, char* argv[]){
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program-to-debug>" << std::endl;
        return 1;
    }

    auto prog = argv[1];
    int pid = fork();
    if(pid == 0) {
        // Child process: execute the program to be debugged
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
        execl(prog, prog, nullptr);
    } else if(pid > 0) {
        // Parent process: create debugger instance and run it
        debugger dbg{prog, pid};
        dbg.run();
    } 
}