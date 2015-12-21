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
#include <ctype.h>
#include <time.h>

/* ===  Globals  ===================================================================== */
int volatile run = 1;
int port = 51717; //51717 instead of 80 is for testing
char* wwwRoot = NULL;
int sockfd;
struct sockaddr_in serv_addr, cli_addr;
FILE* mimefptr;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  intHandler
 *  Description: Handles sigINT (ctrl-c) 
 * =====================================================================================
 */
void intHandler ( int signal );

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
char* makeReturnHeader (const char* status, const char* version, const char* mimeType, int bytes );


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  sendFile
 *  Description: Sends a file to the client 
 * =====================================================================================
 */
int sendFile ( FILE* fptr, int sockfd );


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getMimeType
 *  Description: Finds the mime type needed for a certain route 
 * =====================================================================================
 */
char* getMimeType ( char* path );


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  Initializes the program
 * =====================================================================================
 */
int main ( int argc, char *argv[] ){
	int isDaemon = 0;
	int e;

	//Set up intHander (ctrl-c)
	//Jusgt signal isn't used because it doesn't interrupt accept (restarts it instead)
	struct sigaction a;
	a.sa_handler = intHandler;
	a.sa_flags = 0;
	sigemptyset( &a.sa_mask );
	sigaction( SIGINT, &a, NULL );
	
	//Cycle through passed command options
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
				"Use '-p <number>' to specifiy a port (51717 is the default)\n" \
				"Use 'r <dir>' to specify the www root directory\n" \
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

	//Change root directory
	if(wwwRoot == NULL){
		wwwRoot = (char*)malloc(sizeof(char) * 9);
		strcpy(wwwRoot, "/var/www");
		printf("%s\n", wwwRoot);
		//if(chdir(wwwRoot) < 0){
		//	perror("Failed to change to root directory");
		//}
	}
	else{
		//if(chdir(wwwRoot) < 0){
		//	perror("Failed to change to root directory");
		//}
	}
	initServer(argv[0]);
	//if(chroot(wwwRoot) < 0){
	//	perror("Error changing root directory");
	//	exit(EXIT_FAILURE);
	//}
	//chdir("/");
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
	
	if((mimefptr = fopen("mimeTypes.txt", "r")) == NULL){
		perror("Failed opening mimeType file");
		exit(EXIT_FAILURE);
	}
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
	fclose(mimefptr);
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
	while(run){
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
		if(newsockfd < 0){
			perror("Failed to accept client");
			return;
		}
		else{
			childPid = fork();
			if(childPid < 0){
				perror("Failed to fork process to handle request");
				continue;
			}
			else if(childPid == 0){	
				handleRequest(newsockfd);	
			}
			close(newsockfd);
		}
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
	char status[] = "404 File Not Found";
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
		fclose(mimefptr);
		exit(EXIT_FAILURE);
	}
	//Find the request type (GET, POST, ect)
	if((bytes = sscanf(buffer, "%9s ", typeBuffer)) != 1){
		perror("Failed to find request type");
		close(rsockfd);
		fclose(mimefptr);
		exit(EXIT_FAILURE);
	}//If it is a GET request
	else if(strcmp(typeBuffer, "GET") == 0){
		//Offset to after GET in the request
		position += strlen(typeBuffer) + 1;
		//Read in the file path requested
		if((bytes = sscanf(&buffer[position], "%127s", pathBuffer)) != 1){
			perror("Failed to read file path");
			close(rsockfd);
			fclose(mimefptr);
			exit(EXIT_FAILURE);
		}
		//Check for relative pathing
		if(strstr(pathBuffer, "../") != NULL || strstr(pathBuffer, "..\\") != NULL){
			perror("Path included ../");
			close(rsockfd);
			fclose(mimefptr);
			exit(EXIT_FAILURE);
		}
		//Offset to after the file path
		position += strlen(pathBuffer) + 1;
		//Get the mime type based on what was requested
		char* mimeType = getMimeType(pathBuffer);
		//Error checking
		if(mimeType == NULL){
			close(rsockfd);
			fclose(mimefptr);
			exit(EXIT_FAILURE);
		}
		//Create the full path where the file will be found
		char* fullPath = (char*)malloc(sizeof(char) * 250);
		if(fullPath == NULL){
			perror("Error allocating space");
			free(mimeType);
			close(rsockfd);
			fclose(mimefptr);
			exit(EXIT_FAILURE);
		}
		//Fullpath = wwwRoot + requested path
		if(pathBuffer[0] == '/'){
			sprintf(fullPath, "%s%s", wwwRoot, pathBuffer);
		}
		else{
			sprintf(fullPath, "%s/%s", wwwRoot, pathBuffer);
		}
		printf("Full Path: %s\n", fullPath);
		//Try to open requested file
		if((fptr = fopen(fullPath, "r")) == NULL){
			//If can't open, try and open 404.html instead
			sprintf(fullPath, "%s%s", wwwRoot, "/404.html");	
			printf("%s\n", fullPath);
			if((fptr = fopen(fullPath, "r")) == NULL){
				perror("Error opening file");
				free(mimeType);
				close(rsockfd);
				free(fullPath);
				fclose(mimefptr);
				exit(EXIT_FAILURE);
			}
		}//If opened file correctly, set status to OK
		else{
			sprintf(status, "%s", "200 OK");
		}
		//Get file size and make sure to point at beginning of file
		if(fseek(fptr, 0, SEEK_END) < 0 || (fileSize = ftell(fptr)) < 0 || fseek(fptr, 0, SEEK_SET) < 0){
			perror("Error Determining file size");
			free(mimeType);
			free(fullPath);
			fclose(fptr);
			fclose(mimefptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		//Find the html version from the request
		if((bytes = sscanf(&buffer[position], "%9s", htmlVersionBuffer)) != 1){
			perror("Error getting html version");
			free(mimeType);
			free(fullPath);
			fclose(mimefptr);
			fclose(fptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		//Offset to after the html version in request
		position += strlen(htmlVersionBuffer) + 1;
		//Get the return headers
		htmlHeader = makeReturnHeader(status, htmlVersionBuffer, mimeType, fileSize);
		//Error check
		if(htmlHeader == NULL){
			perror("Error creating return headers");
			free(mimeType);
			free(fullPath);
			fclose(fptr);
			close(rsockfd);
			fclose(mimefptr);
			exit(EXIT_FAILURE);
		}
		//Send the return headers
		write(rsockfd, htmlHeader, strlen(htmlHeader));
		//Send requested file
		if(sendFile(fptr, rsockfd) < 0){
			free(mimeType);
			free(htmlHeader);
			free(fullPath);
			fclose(fptr);
			fclose(mimefptr);
			close(rsockfd);
			exit(EXIT_FAILURE);
		}
		free(mimeType);
		free(htmlHeader);
		free(fullPath);
		fclose(fptr);
		close(rsockfd);
		fclose(mimefptr);
		exit(EXIT_SUCCESS);
	}//If type not supported (not GET request)
	else{
		perror("Unknown request type");
		close(rsockfd);
		fclose(mimefptr);
		exit(EXIT_FAILURE);
	}
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  makeReturnHeader
 *  Description: Creates the html headers to be returned 
 * =====================================================================================
 */
char* makeReturnHeader (const char* status, const char* version, const char* mimeType, int bytes ){
	char* returnHeader;
	time_t rawTime;
	struct tm* timeInfo;
	char* formattedTime;
	//Create return header
	if((returnHeader = (char*)malloc(250 * sizeof(char))) == NULL){
		perror("Error allocating memory for return header");
		return NULL;
	}
	returnHeader[0] = '\0';
	//Find current time
	time(&rawTime);
	timeInfo = localtime(&rawTime);
	formattedTime = asctime(timeInfo);
	//Generate the header and return it
	sprintf(returnHeader, "%s %s\nDate: %sContent-Type: %s\ncharset=UTF-8\nContent-Length: %d\nConnection: close\n\n", version, status, formattedTime, mimeType, bytes);

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
	int buffSize = 1024;
	char buffer[buffSize];
	int bytes = 0;
	//Make sure you are at the start of the file
	if(lseek(filefd, 0, SEEK_SET) < 0){
		return -1;
	}
	do{
		//Readbytes in from the file
		bytes = read(filefd, buffer, (buffSize - 1) * sizeof(char));
		//If at end of file, return
		if(bytes == 0){
			return 0;
		}//Error check
		else if(bytes < 0){
			perror("Error reding from file");
			return -1;
		}
		//Write the read bytes to client
		if(write(sockfd, buffer, bytes) < 0){
			perror("Error writing to socket");
			return -1;
		}
	}while(1);
}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getMimeType
 *  Description: Finds the mime type needed for a certain route 
 * =====================================================================================
 */
char* getMimeType ( char* path ){
	char extension[15];
	char* buffer;
	char* mime;
	char token[48];
	size_t pathSize = 1024; 
	int offset = 0;
	int bytes;
	//Create buffer for mime file
	if((buffer = (char*)malloc(pathSize * sizeof(char))) == NULL){
		perror("Failed allocating space");
		return NULL;
	}
	//Create buffer to hold mime type
	if((mime = (char*)malloc(128 * sizeof(char))) == NULL){
		perror("Failed allocating space");
		free(buffer);
		return NULL;
	}
	//Make sure you are at the begnning of the file
	if(fseek(mimefptr, 0, SEEK_SET) < 0){
		perror("Error seeking in mime file");
		free(buffer);
		free(mime);
		return NULL;
	}
	
	if(sscanf(path, "%*[^.].%14s", extension) == 0){
		//No extension (defaulted to 'text/html'
		sprintf(mime, "text/html");
		free(buffer);
		return mime;
	}
	--pathSize;
	//Read in a line from mime types file
	while((bytes = getline(&buffer, &pathSize, mimefptr)) > 0 && !isspace(buffer[0])){
		//Get the current mime and offset away from it
		sscanf(buffer, "%127s", mime);
		offset = strlen(mime) + 2;
		//Read a file extension for the mime until none are left
		while((sscanf(&buffer[offset], "%47s", token)) == 1){
			offset += strlen(token) + 2;
			//If the exention is the same as requester, return the mime
			if(strcmp(token, extension) == 0){
				free(buffer);
				return mime;
			}
		}
	}
	//mime not found (defaulted to 'text/html'
	sprintf(mime, "text/html");
	free(buffer);
	return mime;
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  intHandler
 *  Description: Handles sigINT (ctrl-c) 
 * =====================================================================================
 */
void intHandler ( int signal ){
	run = 0;	
}
