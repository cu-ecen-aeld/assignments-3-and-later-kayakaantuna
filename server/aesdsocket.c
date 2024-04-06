/**
*
*   AESD Socket application
*
*   Author: Kaya Kaan Tuna <tunakayakaan@gmail.com>
*
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define RET_FAILURE (-1)
#define FAILURE     (1)
#define SUCCESS     (0)
#define PORT        ("9000")
#define DATA_FILE   ("/var/tmp/aesdsocketdata")
#define BUFFER_SIZE (128)

int sockfd;
FILE *file_ptr;
char* ip_address;

void signalhandler (int signal)
{
    if (signal == SIGINT || signal == SIGTERM )
    {
        syslog(LOG_INFO, "Caught signal, exiting");
        close(sockfd);
        remove(DATA_FILE);
        closelog();
        exit(SUCCESS);
    }
}

int main(int argc, char* argv[])
{
    /*      VARIABLES       */
    struct addrinfo hints, *res;
    struct sockaddr_in clientaddr;
    socklen_t clientaddrlen;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    int daemon = 0;
    int rc = 0;
    int newfd;

    int option_value=1; // For setsockopt

    int backlog = 10; // Max length

    /************************/


    openlog(NULL, LOG_PID, LOG_USER);
    syslog(LOG_USER,"Success: Starting log...\n");

     if((argc > 1) && (strcmp(argv[1], "-d") == 0))
     {
     	daemon = 1; 
     	syslog(LOG_USER,"Success: Running in daemon mode...\n");     	
     }

    if (signal(SIGINT, signalhandler ) == SIG_ERR)
    {
        syslog(LOG_ERR, "Error signal handler for SIGNINT\n");
        closelog();
        exit(FAILURE);
    }

    if (signal(SIGTERM, signalhandler ) == SIG_ERR )
    {
        syslog(LOG_ERR, "Error signal handler for SIGNTERM\n");
        closelog();
        exit(FAILURE);
    }

    rc = getaddrinfo(NULL, PORT, &hints, &res);
    if (rc != SUCCESS)
    {
        syslog(LOG_ERR,"Error getting address info; getaddrinfo() failure\n");
        printf("Error! getaddrinfo() failure\n");
        freeaddrinfo(res);
        closelog();
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: getaddrinfo()\n");

    if (res == NULL)
    {
        syslog(LOG_ERR,"Error storing info in res; malloc() failure\n");
        printf("Error! malloc() failure\n");
        freeaddrinfo(res);
        closelog();
        exit(FAILURE);    
    }

    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error creating socket; socket() failure\n");
        printf("Error! socket() failure\n");
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);
    }
    syslog(LOG_INFO,"Success: socket()\n");

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) == -1) 
    {
        syslog(LOG_ERR,"Error setting socket options; setsockopt() failure\n");
        printf("Error! setsockopt() failure\n");
        freeaddrinfo(res);
        closelog();
        close(sockfd);
    	exit(FAILURE);
    }

    if (daemon)
    {
        pid_t pid = fork();

    	if (pid == RET_FAILURE)
    	{
        	syslog(LOG_ERR, "Error forking: %m");
        	close(sockfd);
    		closelog();
        	exit(FAILURE);
    	} 	
    	else if (pid != 0)
    	{
            close(sockfd);
    		closelog();
        	exit(SUCCESS);
    	}
    }

    rc = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if (rc == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error binding to the port we passed in to getaddrinfo(); bind() failure\n");
        printf("Error! bind() failure\n");
        freeaddrinfo(res);
        closelog();
        close(sockfd);
        exit(FAILURE);       
    }
    syslog(LOG_INFO,"Success: bind()\n");
    freeaddrinfo(res);


    rc = listen(sockfd, backlog);
    if (rc == RET_FAILURE)
    {
        syslog(LOG_ERR,"Error listening for connections on a socket; listen() failure\n");
        printf("Error! listen() failure\n");
        closelog();
        exit(FAILURE);        
    }
    syslog(LOG_INFO,"Success: listen()\n");

    while(1)
    {
        clientaddrlen = sizeof(clientaddr);
        newfd =  accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (newfd == RET_FAILURE)
        {
            syslog(LOG_ERR,"Error accepting a connection on a socket; accept() failure\n");
            printf("Error! accept() failure\n");
            closelog();
            exit(FAILURE);       
        }
        syslog(LOG_INFO,"Success: accept()\n");

        ip_address = inet_ntoa(clientaddr.sin_addr);
        syslog(LOG_USER,"Accepted connection from %s\n", ip_address);

        if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL)
		{
        	syslog(LOG_ERR,"Error while opening given file!\n");
			printf("Error! Opening given file\n");
			return FAILURE;
		}
    	char buffer[BUFFER_SIZE];

        while(1)
        {
        	int num_bytes = recv(newfd, buffer, sizeof(buffer), 0);
        	if (num_bytes == RET_FAILURE)
        	{
        		syslog(LOG_ERR,"Error while receiving!\n");
                printf("Error! Receiving/n");	
                break;
        	}

        	fwrite(buffer, 1, num_bytes, file_ptr);

        	if (memchr(buffer, '\n', num_bytes) != NULL)
            	break;
        }

        fclose(file_ptr);

        if ((file_ptr = fopen(DATA_FILE, "r")) == NULL)
        {
            syslog(LOG_ERR,"Error while opening given file!\n");
            printf("Error! Opening given file\n");
            return FAILURE;
        }

    	while (!feof(file_ptr))
    	{
        	ssize_t read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr);
        	if (read_bytes <= 0) 
            		break;

        	send(newfd, buffer, read_bytes, 0);
    	}
    	fclose(file_ptr);
    }

    syslog(LOG_USER, "Closed connection from %s\n", ip_address);
    close(sockfd);
    close(newfd);
    closelog();
    remove(DATA_FILE);
    return(SUCCESS);
}
