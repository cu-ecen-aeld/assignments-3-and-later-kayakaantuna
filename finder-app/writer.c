#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FILE *fp;

    openlog("writer", LOG_PID, LOG_USER);

    // Check if the number of arguments is correct
    if (argc != 3) {
        printf("Error: Two arguments required. The path to the file and the string to write.\n");
        syslog(LOG_ERR, "Invalid number of arguments. Expected 2, got %d.", argc-1);
        closelog();
        return 1;
    }

    fp = fopen(argv[1], "w");
    if (fp == NULL) {
        printf("Error: Could not open file %s for writing.\n", argv[1]);
        syslog(LOG_ERR, "Could not open file %s for writing.", argv[1]);
        closelog();
        return 1;
    }

    fprintf(fp, "%s", argv[2]);
    fclose(fp);

    syslog(LOG_DEBUG, "Writing '%s' to %s", argv[2], argv[1]);

    closelog();
    
    return 0;
}
