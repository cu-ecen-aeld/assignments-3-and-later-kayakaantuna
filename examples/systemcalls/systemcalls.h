#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h> // For fork(), execv()
#include <sys/wait.h> // For wait()
#include <stdarg.h> // For variadic functions
#include <stdbool.h> // For bool type
#include <stdlib.h> // For system()
#include <fcntl.h>

bool do_system(const char *command);

bool do_exec(int count, ...);

bool do_exec_redirect(const char *outputfile, int count, ...);
