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
#include <signal.h>

/* ===  Globals  ===================================================================== */
int port = 51717; //51717 instead of 80 is for testing
char* wwwRoot = NULL;
int sockfd;
struct sockaddr_in serv_addr, cli_addr;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  becomeDaemon
 *  Description: Makes the program into a daemon 
 * =====================================================================================
 */
void becomeDaemon ();


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  initServer
 *  Description: Initializes the server 
 * =====================================================================================
 */
void initServer (const char* pName);


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  startServer
 *  Description: Begins accepting new clients 
 * =====================================================================================
 */
void startServer ();


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  closeServer
 *  Description: Closes the server 
 * =====================================================================================
 */
void closeServer ();



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  handleRequest
 *  Description: Handles an http request 
 * =====================================================================================
 */
void handleRequest ( int rsockfd );


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  makeReturnHeader
 *  Description: Creates the html headers to be returned 
 * =====================================================================================
 */
char* makeReturnHeader (const char* status, const char* version, int bytes );


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  sendFile
 *  Description: Sends a file to the client 
 * =====================================================================================
 */
int sendFile ( FILE* fptr, int sockfd );


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
	signal(SIGCHLD, SIG_IGN);
	if(isDaemon){
		becomeDaemon(argv[0]);
	}

	//Change roo directory
	if(wwwRoot == NULL){
		wwwRoot = (char*)malloc(sizeof(char) * 9);
		strcpy(wwwRoot, "/var/www");
		printf("%s\n", wwwRoot);
		if(chdir(wwwRoot) < 0){
			perror("Failed to change to root directory");
		}
	}
	else{
		if(chdir(wwwRoot) < 0){
			perror("Failed to change to root directory");
		}
	}
	initServer(argv[0]);
	startServer();
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
void becomeDaemon (){
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
void initServer (const char* pName){
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
	printf("\nListening on port: %d\n" \
		"Daemon Process Name: %s\n", port, pName);
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


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  startServer
 *  Description: Begins accepting new clients 
 * =====================================================================================
 */
void startServer (){
	int newsockfd;
	socklen_t clilen;
	pid_t childPid;
	while(1){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
		if(newsockfd < 0){
			perror("Failed to accept client");
		}
		childPid = fork();
		if(childPid < 0){
			perror("Failed to fork process to handle request");
			continue;
		}
		else if(childPid == 0){	
			handleRequest(newsockfd);	
			close(newsockfd);
			exit(EXIT_SUCCESS);
		}
		close(newsockfd);
	}
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  handleRequest
 *  Description: Handles an http request 
 * =====================================================================================
 */
void handleRequest ( int rsockfd ){	
	char buffer[1024];
	char typeBuffer[10];
	char pathBuffer[128];
	char htmlVersionBuffer[10];
	char* htmlHeader;
	int bytes;
	int position = 0;
	FILE* fptr;
	int fileSize;

	//Read in the request
	bzero(buffer, 1024);
	if((bytes = read(rsockfd, buffer, 1023)) <= 0){
		perror("Error reading from socket");
		close(rsockfd);
		exit(EXIT_FAILURE);
	}
	//Find the request type (GET, POST, ect)
	if((bytes = sscanf(buffer, "%9s ", typeBuffer)) != 1){
		perror("Failed to find request type");
		close(rsockfd);
		exit(EXIT_FAILURE);
	}
	else if(strcmp(typeBuffer, "GET") == 0){
		position += strlen(typeBuffer) + 1;
		if((bytes = sscanf(&buffer[position], "%127s", pathBuffer)) != 1){
			perror("Failed to read file path");
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		position += strlen(pathBuffer) + 1;
		char* fullPath = (char*)malloc(sizeof(char) * 250);
		if(fullPath == NULL){
			perror("Error allocating space");
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		memcpy(fullPath, wwwRoot, strlen(wwwRoot));
		memcpy(&fullPath[strlen(wwwRoot)], &pathBuffer, sizeof(pathBuffer));
		if((fptr = fopen(fullPath, "r")) == NULL){
			perror("Error opening file");
			close(rsockfd);
			free(fullPath);
			exit(EXIT_FAILURE);
		}
		if(fseek(fptr, 0, SEEK_END) < 0 || (fileSize = ftell(fptr)) < 0 || fseek(fptr, 0, SEEK_SET) < 0){
			perror("Error Determining file size");
			free(fullPath);
			fclose(fptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		if((bytes = sscanf(&buffer[position], "%9s", htmlVersionBuffer)) != 1){
			perror("Error getting html version");
			free(fullPath);
			fclose(fptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		position += strlen(htmlVersionBuffer) + 1;
		htmlHeader = makeReturnHeader("200 OK", htmlVersionBuffer, fileSize);
		if(htmlHeader == NULL){
			perror("Error creating return headers");
			free(fullPath);
			fclose(fptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		write(rsockfd, htmlHeader, strlen(htmlHeader));
		if(sendFile(fptr, rsockfd) < 0){
			perror("Error sending file");
			free(htmlHeader);
			free(fullPath);
			fclose(fptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		free(htmlHeader);
		free(fullPath);
		fclose(fptr);
	}
	else{
		perror("Unknown request type");
		close(rsockfd);
		exit(EXIT_FAILURE);
	}
	/*printf("%s\n", typeBuffer);
	
	if((bytes = write(rsockfd, "Recieved message", 17)) < 0){
		perror("Error writing to socket");
		close(rsockfd);
		exit(EXIT_FAILURE);
	}*/
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  makeReturnHeader
 *  Description: Creates the html headers to be returned 
 * =====================================================================================
 */
char* makeReturnHeader (const char* status, const char* version, int bytes ){
	char* returnHeader;
	if((returnHeader = (char*)malloc(250 * sizeof(char))) == NULL){
		perror("Error allocating memory for return header");
		return NULL;
	}
	returnHeader[0] = '\0';
	sprintf(returnHeader, "%s %s\nDate: %s\nContent-Type: text/html\ncharset=UTF-8\n Content-Length: %d\nConnection: close\n\n", version, status, "Temp", bytes);
	return returnHeader;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  sendFile
 *  Description: Sends a file to the client 
 * =====================================================================================
 */
int sendFile ( FILE* fptr, int sockfd ){
	int filefd = fileno(fptr);
	char buffer[1024];
	int bytes = 0;

	if(lseek(filefd, 0, SEEK_SET) < 0){
		return -1;
	}
	do{
		bytes = read(filefd, buffer, 1023 * sizeof(char));
		if(bytes == 0){
			return 0;
		}
		write(sockfd, buffer, bytes);
	}while(1);
}
