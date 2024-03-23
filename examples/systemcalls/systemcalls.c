#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    if (cmd == NULL) {
        // The system() specification states that if cmd is NULL,
        // the function returns nonzero if a command processor is available,
        // and zero otherwise. This behavior does not indicate success or failure
        // of a command execution, so it's handled separately.
        return false;
    }
/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    int ret = system(cmd);

    // Check if the command was executed successfully
    if (ret == -1) {
        // system() call failed
        return false;
    } else if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        // The command was executed successfully
        return true;
    } else {
        // The command failed or system() didn't execute properly
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    // Forking
    pid_t pid = fork();

    if (pid == -1)
    {
        // Fork failed
        va_end(args);
        return false;
    }
    else if (pid == 0)
    {
        // Child process
        execv(command[0], command);
        // If execv returns, it must have failed
        _exit(EXIT_FAILURE);
    }
    else
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        va_end(args);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            // Command executed successfully
            return true;
        }
        else
        {
            // Command execution failed
            return false;
        }
    }
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char *command[count + 1];
    for (int i = 0; i < count; i++) {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    int fd = open(outputfile, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fd < 0) { 
        perror("open"); 
        va_end(args);
        return false; 
    }

    pid_t pid = fork();

    if (pid == -1) {
        // Fork failed
        close(fd);
        va_end(args);
        return false;
    } else if (pid == 0) {
        // Child process
        if (dup2(fd, STDOUT_FILENO) < 0) { // Redirect stdout to the file
            perror("dup2");
            close(fd);
            _exit(EXIT_FAILURE);
        }
        close(fd); // No longer need the original fd
        execv(command[0], command);
        // If execv returns, it has failed
        perror("execv");
        _exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(fd); // Close the file descriptor as it's no longer needed in the parent
        int status;
        waitpid(pid, &status, 0);
        va_end(args);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Command executed successfully
            return true;
        } else {
            // Command execution failed
            return false;
        }
    }
}