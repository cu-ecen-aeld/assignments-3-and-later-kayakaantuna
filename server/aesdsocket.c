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
#include <pthread.h>
#include <stdbool.h>
#include "queue.h"

#define RET_FAILURE (-1)
#define FAILURE     (1)
#define SUCCESS     (0)
#define PORT        ("9000")
#define DATA_FILE   ("/var/tmp/aesdsocketdata")
#define BUFFER_SIZE (128)
#define RFC2822_compliant_strftime_format ("%a, %d %b %Y %T %z\n")

int sockfd;
FILE *file_ptr;
char* ip_address;
pthread_mutex_t lock;
bool signal_exit = false;
bool timer_exit = false;

struct timespec time_now, time_sleep = {10, 0};
struct tm time_info;

typedef struct slist_client_s slist_client_t;
struct slist_client_s 
{
    pthread_t thread_id;
    bool thread_completion_flag;
    int newfd;
    SLIST_ENTRY(slist_client_s) entries;
};

SLIST_HEAD(slisthead, slist_client_s) head;

void cleanup()
{
    remove(DATA_FILE);
    syslog(LOG_INFO, "Caught signal, exiting");
    closelog();
    struct slist_client_s *new_client_node, *temp;
    SLIST_FOREACH_SAFE(new_client_node, &head, entries, temp)
    {
        pthread_join( new_client_node->thread_id, NULL );
        SLIST_REMOVE( &head, new_client_node, slist_client_s, entries );
        free( new_client_node);
    }
    exit(SUCCESS);
}

void signalhandler (int signal)
{ 
    if (signal == SIGINT || signal == SIGTERM )
    {   
        signal_exit = true;
        shutdown(sockfd, SHUT_RDWR);
        cleanup();
    }   
}

void timehandler(int signal)
{ 
    if (signal == SIGALRM)
    {          
        timer_exit = true;
    }   
}

void *multithread_handler(void *new_client_node)
{
	struct slist_client_s *thread_param = (struct slist_client_s*)new_client_node;
    pthread_mutex_lock(&lock);
    if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL)
	{
    	syslog(LOG_ERR,"Error while opening given file!\n");
		printf("Error! Opening given file\n");
		pthread_exit(NULL);
	}
    char buffer[BUFFER_SIZE];
    
    while(1)
    {
    	int num_bytes = recv(thread_param->newfd, buffer, sizeof(buffer), 0);
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
    pthread_mutex_unlock(&lock);
    
    pthread_mutex_lock(&lock);
    if ((file_ptr = fopen(DATA_FILE, "r")) == NULL)
	{
    	syslog(LOG_ERR,"Error while opening given file!\n");
		printf("Error! Opening given file\n");
		pthread_exit(NULL);
	}
	
	while (!feof(file_ptr))
	{
    	ssize_t read_bytes = fread(buffer, 1, sizeof(buffer), file_ptr);
    	if (read_bytes <= 0) 
        	break;

    	send(thread_param->newfd, buffer, read_bytes, 0);
	}
	
	fclose(file_ptr); 
	pthread_mutex_unlock(&lock);
	thread_param->thread_completion_flag = 1;
	pthread_exit(NULL);
}

void *timestamp_handler (void *arg)
{
    char buffer[BUFFER_SIZE];

    while (!timer_exit)
    {
        clock_gettime(CLOCK_REALTIME, &time_now);       
        localtime_r( &time_now.tv_sec, &time_info); 
        strftime(buffer, sizeof(buffer), RFC2822_compliant_strftime_format, &time_info);

        pthread_mutex_lock(&lock);
        
        if ((file_ptr = fopen(DATA_FILE, "a+")) == NULL)
		{
        	syslog(LOG_ERR,"Error while opening given file!\n");
			printf("Error! fopen() failure\n");
			pthread_mutex_unlock(&lock);
			pthread_exit(NULL);
		}
        
        fwrite("timestamp:", 1, strlen("timestamp:"), file_ptr);
        fwrite(buffer, 1, strlen(buffer), file_ptr);

        fclose(file_ptr);
        pthread_mutex_unlock(&lock);
        
        nanosleep(&time_sleep, NULL);
    }
    pthread_exit(NULL);
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

    if (signal(SIGALRM, timehandler) == SIG_ERR)
    {
        syslog(LOG_ERR, "Error signal handler for SIGALRM\n");
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
    syslog(LOG_USER,"Success: setsockopt()\n");

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
    	    syslog(LOG_INFO,"Success: fork()\n");
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

    rc = pthread_mutex_init(&lock, NULL);
    if (rc != SUCCESS)
    {
        syslog(LOG_ERR,"Error initializing mutex; pthread_mutex_init() failure\n");
        printf("Error! pthread_mutex_init() failure\n");
        closelog();
        exit(FAILURE);                
    }
    SLIST_INIT(&head);

    pthread_t timestamp_thread;
    rc = pthread_create(&timestamp_thread, NULL, timestamp_handler, NULL);
    if (rc != SUCCESS)
    {
        syslog(LOG_ERR,"Error creating timestamp thread; pthread_create() failure\n");
        printf("Error! pthread_create() failure\n");
        closelog();
        exit(FAILURE);                
    }

    while(!signal_exit)
    {
        clientaddrlen = sizeof(clientaddr);
        int newfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
        if (newfd == RET_FAILURE)
        {
            syslog(LOG_ERR,"Error accepting a connection on a socket; accept() failure\n");
            closelog();
            exit(FAILURE);       
        }
        syslog(LOG_INFO,"Success: accept()\n");

        ip_address = inet_ntoa(clientaddr.sin_addr);
        syslog(LOG_USER,"Accepted connection from %s\n", ip_address);

        struct slist_client_s *new_client_node = (struct slist_client_s*) malloc(sizeof(struct slist_client_s));
        new_client_node->newfd = newfd;
        new_client_node->thread_completion_flag = 0;
        SLIST_INSERT_HEAD(&head, new_client_node, entries);

        rc = pthread_create(&new_client_node->thread_id, NULL, multithread_handler, (void *)new_client_node);
        if (rc != SUCCESS)
        {
            syslog(LOG_ERR,"Error creating thread; pthread_create() failure\n");
            printf("Error! pthread_create() failure\n");
            SLIST_REMOVE(&head, new_client_node, slist_client_s, entries);
            close(new_client_node->newfd);
            free(new_client_node);
            closelog();
            exit(FAILURE);                
        }
        syslog(LOG_INFO,"Success: pthread_create()\n");

        struct slist_client_s *nextThread;
        SLIST_FOREACH_SAFE(new_client_node, &head, entries, nextThread)
        {
             if (new_client_node->thread_completion_flag)
             {
                 rc = pthread_join(new_client_node->thread_id, NULL);
                 SLIST_REMOVE(&head, new_client_node, slist_client_s, entries);
                 free(new_client_node);
                 if (rc != SUCCESS)   
                 {
                     syslog(LOG_ERR,"Error joining thread; pthread_join() failure\n");
                     printf("Error! pthread_join() failure\n");                         
                     closelog();
                     exit(FAILURE);  
                 }
             }
        }
        syslog(LOG_USER, "Closed connection from %s\n", ip_address);
    }
}
