/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  A simple webserver written in C. The goal is to be able to handle
 *                  http requests safely.
 *
 *        Version:  1.0
 *        Created:  12/03/15 22:45:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Bobjrsenior (BOB), bobjrsenior@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* ===  Globals  ===================================================================== */
int port = 51717; //51717 instead of 80 is for testing
char* wwwRoot = NULL;
int sockfd, newsockfd;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  becomeDaemon
 *  Description: Makes the program into a daemon 
 * =====================================================================================
 */
void becomeDaemon (const char* pName);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  initServer
 *  Description: Initializes the server 
 * =====================================================================================
 */
void initServer ();


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  closeServer
 *  Description: Closes the server 
 * =====================================================================================
 */
void closeServer ();


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  Initializes the program
 * =====================================================================================
 */
int main ( int argc, char *argv[] ){
	int isDaemon = 0;

	//Cycle through passed command options
	int e;
	for(e = 1; e < argc; ++e){
		//'-d' is to make the program a daemon
		if(strcmp(argv[e], "-d") == 0){
			isDaemon = 1;
		}//'-p' s to specify a port
		else if(strcmp(argv[e], "-p") == 0){
			if(++e < argc){
				port = atoi(argv[e]);
			}
		}//'-r' is to set the www root directory
		else if(strcmp(argv[e], "-r") == 0){
			if(++e < argc){
				wwwRoot = argv[e];
			}		
		}//'-help' shows all available commands
		else if(strcmp(argv[e], "-help") == 0){
			printf("Command Options\n" \
				"Use '-d' to run as a daemon\n" \
				"Use '-p <number>' to specifiy a port\n" \
				"Use 'r <dir>' to specify to www root directory\n" \
				"Use '-help' to see this output\n");			
			return EXIT_SUCCESS;
		}//If invalid command
		else{
			printf("Invalid command '%s' specified\n" \
				"Use command '-help' to see available options\n", argv[e]);
			return EXIT_FAILURE;
		}
	}
	if(isDaemon){
		becomeDaemon(argv[0]);
	}

	//Change roo directory
	if(wwwRoot == NULL){
		if(chdir("/var/www/") < 0){
			perror("Failed to change to root directory");
		}
	}
	else{
		if(chdir(wwwRoot) < 0){
			perror("Failed to change to root directory");
		}
	}
	initServer();
	closeServer();
	printf("Success\n");
	return EXIT_SUCCESS;
}/* ----------  end of function main  ---------- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  becomeDaemon
 *  Description: Makes the program into a daemon 
 * =====================================================================================
 */
void becomeDaemon (const char* pName){
	pid_t childPid, sid;

	//You are already a daemon
	if(!getppid()){
		return;
	}

	//Fork and make the parent exit
	childPid = fork();
	if(childPid < 0){
		perror("Failed to fork into daemon");
		exit(EXIT_FAILURE);
	}
	else if(childPid){
		printf("Process Name: %s\nProcess ID: %d\n", pName, getpid());
		exit(EXIT_SUCCESS);
	}

	//Make a new session	
	if((sid = setsid()) < 0){
		perror("Failed to make a new session for daemon");
	} 

}	


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  initServer
 *  Description: Initializes the server 
 * =====================================================================================
 */
void initServer (){
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Failed to open socket");
		exit(EXIT_FAILURE);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if((bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0){
		perror("Failed to bind port");
		exit(EXIT_FAILURE);
	}
	listen(sockfd, 5);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  closeServer
 *  Description: Closes the server 
 * =====================================================================================
 */
void closeServer (){
	close(sockfd);
}
