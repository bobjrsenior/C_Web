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

/* ===  Globals  ===================================================================== */



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  becomeDaemon
 *  Description: Makes the program into a daemon 
 * =====================================================================================
 */
void becomeDaemon (const char* pName);

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  Initializes the program
 * =====================================================================================
 */
int main ( int argc, char *argv[] ){
	becomeDaemon(argv[0]);
	sleep(10);
	printf("I am daemon\n");
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
		printf("%s\n", pName);
		exit(EXIT_SUCCESS);
	}

	//Make a new session	
	if((sid = setsid()) < 0){
		perror("Failed to make a new session for daemon");
	} 

	//Change roo directory
	////TODO: Make into wwwroot
	if(chdir("/") < 0){
		perror("Failed to change to root directory");
	}
}	
	
