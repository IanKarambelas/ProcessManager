/******************************************************************
 * Ian Karambelas
 * CIS 452
 * Program 2
 * 2/14/13
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXSERVERS 9
#define MAXCHILDREN 9

//struct which defines the variables associated with a server
typedef struct server {
	char* name;					//name of the server
	pid_t pid;					//pid of the server
	int minProcs;				//minumum number of child processes
	int maxProcs;				//maximum number of child processes
	int children;				//number of child processes
	pid_t child[MAXCHILDREN];	//an array to hold the pids of children
} Server;

//array to hold server structs
Server servers[MAXSERVERS];

//interrupt kills everything, just in case
void genocide(int signum) {
	printf("\t Failsafe activated, committing genocide...\n");
	exit(signum);
}

//handler for SIGUSR1 that spawns a child process
void spawn(int signum) {
		
	pid_t child;	//holds the child pid
	int i;			//counter

	//figure out which server this is
	for (i = 0; servers[i].pid != getpid(); i++);
	printf("Executing spawn on server %s PID# %d\n",
		servers[i].name, servers[i].pid);

	//spawns a child
	if ((child = fork()) < 0) {
		perror("fork failure");
			exit(1);
	}

	//child spins forever until killed
	if (child == 0) {
		pause();
	}

	//server code
	else {
		
		//tell the user the child was spawned
		printf("Child PID# %d spawned by server %s PID# %d\n",
			child, servers[i].name, getpid());
		
		//increment the child counter
		servers[i].children++;
		
		//add the child pid to the server's array of children
		servers[i].child[servers[i].children - 1] = child;
		
		//tell the user how many children the server has
		printf("Server %s now has %d children\n", 
			servers[i].name, servers[i].children);
	
	}
	
	//tell the process manager the server is done spawning
	kill(getppid(), SIGCONT);

	//reset the signal
	signal(SIGUSR1, spawn);

}


//handler for SIGUSR2 which kills a child process
void infanticide(int signum) {

	int i;		//counter
	int status;	//status return value
	
	//figure out which server this is
	for (i = 0; servers[i].pid != getpid(); i++);
	printf("Executing infanticide on server %s PID# %d\n",
		servers[i].name, servers[i].pid);
	
	//kill the child at the end of the child pid array
	kill(servers[i].child[servers[i].children - 1], SIGKILL);
	//reap it
	wait(&status);
	
	//inform the user that the child has been killed
	printf("Infanticide has been performed on child PID# %d with status: %d\n", 
		servers[i].child[servers[i].children - 1], status);
	
	//set the pid at the child's former location in the server array to -1
	servers[i].child[servers[i].children - 1] = -1;
	
	//decrement the number of children
	servers[i].children--;
	
	//inform the user of how many children the server has
	printf("Server %s now has %d children\n", 
			servers[i].name, servers[i].children);
	
	//tell the process manager the server is done killing the child
	kill(getppid(), SIGCONT);
		
	//reset the signal
	signal(SIGUSR2, infanticide);
	
}

//kills all children and then self
void murder_suicide(int signum) {

	int i;		//counter
	int status;	//status return value
	
	//figure out which server this is
	for (i = 0; servers[i].pid != getpid(); i++);
	printf("Executing murder/suicide on server %s PID# %d\n",
		servers[i].name, servers[i].pid);
	
	//while the server still has children
	while(servers[i].children > 0) {
		
		//kill the child at the end of the array
		kill(servers[i].child[servers[i].children - 1], SIGKILL);
		//reap it
		wait(&status);
		
		//inform the user that the child has been killed
		printf("Infanticide has been performed on child PID# %d with status: %d\n", 
			servers[i].child[servers[i].children - 1], status);
		
		//set its pid to -1
		servers[i].child[servers[i].children - 1] = -1;
	
		//decrement the number of children
		servers[i].children--;
		
		//inform the user of how many children there are
		printf("Server %s now has %d children\n", 
			servers[i].name, servers[i].children);
	
	}

	//inform the user that all the server's children have been killed
	printf("%s has killed all of its children\n", servers[i].name);
	printf("%s will now kill itself\n", servers[i].name);

	//reset the signal
	signal(SIGTERM, murder_suicide);
	
	//kill self
	exit(0);

}

//nothing goes here, this tells the process manager that a server is done
//spawning or killing children
void cont(int signum){

	//reset the signal
	signal(SIGCONT, cont);

}

//prints a table displaying the relationship heirarchy
void printchildren(int signum) {

	int i, j;	//counters
	
	//figure out which server this is
	for (i = 0; servers[i].pid != getpid(); i++);
	
	//print the server pid and name
	printf("%d\t\\_%s\n", servers[i].pid, servers[i].name);
	
	//print the childrens' pids and their numbers
	for (j = 0; j < servers[i].children; j++) {
	
		printf("%d\t  \\_ Child #%d\n", servers[i].child[j], j + 1);
	
	} 

	//tell the process manager this server is done printing (this prevents
	//another server from beginning to print before this one is done)
	kill(getppid(), SIGCONT);
	
	//reset the signal
	signal(SIGALRM, printchildren);
	
}

//handles the server
void serverHandler() {
		
	//debug statement
	//printf("Server PID# %d has been caught by the handler!\n", getpid());

	//wait for signals until killed
	while(1)
		pause();

}

//spawns a child that waits between 1 and 5 seconds, then
//sends SIGUSR1 or SIGUSR2 to the parent process
int main(int argc, char* argv[]) {

	char* command;      //the user entered command
	char* input[256];   //an array to tokenize the command and parameters
	char buf[256];      //a buffer array
	pid_t pid;          //temp pid
	int status;         //a place to store the status of the child on exit
	char* pEnd;      	//a pointer for use with the strtol function
	int i, j;			//counters

	//signal set up
	signal(SIGINT, genocide);
	signal(SIGUSR1, spawn); 
	signal(SIGUSR2, infanticide);
	signal(SIGTERM, murder_suicide);
	signal(SIGCONT, cont);
	signal(SIGALRM, printchildren);
		
	//continue to take user commands
	while(1) {
				
		printf("Enter command: ");

		//gets user input
		command = fgets(buf, 255, stdin);
		input[0] = strtok(command, " \n");
		input[1] = strtok(NULL, " \n");
		input[2] = strtok(NULL, " \n");
		input[3] = strtok(NULL, " \n");
		input[4] = NULL;

		//if the user enters nothing
		if (input[0] == NULL) {
			printf("must supply a command. Please RTFM.\n");
		}

		//if the user decides to create a new server
		else if (strcmp(input[0], "createServer") == 0) {
			
			//if the user enters no server name
			if (input[1] == NULL || input[2] == NULL || input[3] == NULL) {
				printf("Not enough arguments. Please RTFM.\n");
				i = 99;
			}
			
			//"NULL" is a protected name and cannot be assigned to servers
			//this is because "NULL" is used to tell if a server has been deleted
			else if ( strcmp("NULL", input[1]) == 0) {
				printf("Invalid server name. Please RTFM\n");
				i = 99;
			}
			
			//if the user enters bad numbers for the min and max procs
			else if (atoi(input[1]) < 1 || atoi(input[2]) > MAXCHILDREN
					|| atoi(input[1]) > atoi(input[2]) ) {
				printf("Invalid number of processes entered. Please RTFM.\n");
				i = 99;
			}
		   
			//step through the array of servers to find an empty one
			//"NULL" as a name indicates the server was deleted and the space
			//can be reused
			else {
				for (i = 0; servers[i].pid > 0; i++);
			}
			
			//break out of an error occurred or if i incremented past the maximum
			//number of servers
			if(i >= MAXSERVERS)
				printf("Could not spawn server\n");
			
			else {
			
				//spawns a server
				if ((pid = fork()) < 0) {
					perror("fork failure");
					exit(1);
				}
								
				//inits all of the server values
				servers[i].minProcs = strtol(input[1],&pEnd,10);
				servers[i].maxProcs = strtol(input[2],&pEnd,10);
				servers[i].name = malloc(sizeof(input[3]));
				strcpy(servers[i].name, input[3]);
				servers[i].pid = pid;
				servers[i].children = 0;
				for (j = 0; j < MAXCHILDREN; j++)                               
					servers[i].child[j] = -1;

				//newly spawned server
				if (pid == 0) {
				
					//correctly sets the pid (new process has its own variables)
					servers[i].pid = getpid();

					//go to server function
					serverHandler();

				}
									
				//process manager
				else {
					//informs the user that the server was spawned
					printf("Spawned server %s PID# %d\n", 
						servers[i].name, servers[i].pid);
						
				}
								
			}
						
		} //end createServer

		//if the user chooses to kill a server
		else if (strcmp(input[0], "abortServer") == 0) {
			
			//if the user entered no server name 
			if (input[1] == NULL) {
				printf("You must enter a server name. Please RTFM\n");
				i = 99;
			}

			//finds the correct server
			else
				for (i = 0; (strcmp(servers[i].name, input[1]) != 0); i++);
						
			//if incremented past
			if(i >= MAXSERVERS)
				printf("Server not found. Check input name.\n");

			//if found
			else {
			
				//tells the server to kill all of its children and then itself
				kill(servers[i].pid, SIGTERM);
				//reaps the server
				wait(&status);
				
				//informs the user that the server has been killed
				printf("%s has died with status: %d\n", servers[i].name, status);
				
				//marks the server variables so that it may be reused
				servers[i].name = "NULL";
				servers[i].pid = -1;
			 
			 }

		} //end abortserver
				
		//if the user chooses to display the process hierarchy
		else if (strcmp(input[0], "displayStatus") == 0) {
			
			//prints titles
			printf(" PID\tCOMMAND\n");
			//prints the process manager and its pid
			printf("%d\tProcMan\n", getpid());
			
			//prints all of the servers and their children, one by one
			for(i = 0; i < MAXSERVERS; i++){
				if (servers[i].pid > 0) {
					kill(servers[i].pid, SIGALRM);
					pause();
					
				}
			
			}
		
		} //end display

		//if the user chooses to spawn a child from a server
		else if (strcmp(input[0], "createProc") == 0) {
			 
			//if the user inputs no server name
			if (input[1] == NULL) {
				printf("You must enter a server name. Please RTFM\n");
				i = 99;
			}
			
			//finds the server by name
			else
				for (i = 0; (strcmp(servers[i].name, input[1]) != 0); i++);
	
			//if not found
			if (i >= MAXSERVERS)
				printf("Server not found.\n");
			
			else {
			
				//informs the user the server is being instructed to spawn a child
				printf("Sending spawn signal to server %s PID# %d\n",
					servers[i].name, servers[i].pid);

				//sends the spawn signal to the server
				kill(servers[i].pid, SIGUSR1);
				pause();

				//increments the number of children it has
				servers[i].children++;

			}

		} //end createProc
				
		//if the user chooses to end a child process
		else if (strcmp(input[0], "abortProc") == 0) {
			
			//if the user does not input a server name
			if (input[1] == NULL) {
				printf("You must enter a server name. Please RTFM\n");
				i = 99;
			}
			
			//finds the server by name
			else {
				for (i = 0; (strcmp(servers[i].name, input[1]) != 0); i++);
			}
			
			//if not found
			if (i >= MAXSERVERS)
				printf("Server not found.\n");          
			
			//if the server has no children (somehow)
			else if (servers[i].children < 1) 
				printf("Server %s has no children to kill\n", servers[i].name);
				
			else {
				//informs the user the server has been instructed to kill a child
				printf("Sending infanticide signal to server %s PID# %d\n",
					servers[i].name, servers[i].pid);

				//sends the infanticide signal to the server
				kill(servers[i].pid, SIGUSR2);
				pause();

				//decrements the number of children it has
				servers[i].children--;

			}

		}
		
		//if the user enters a bad command
		else {
			printf("You must enter a valid command. Please RTFM.\n");
		}
		
		//checks all servers to ensure that the number of children they have
		//is within range
		for(i = 0; i < MAXSERVERS; i++) {
			
			//if there is a valid server at this location
			if (servers[i].pid > 0) {
				
				//while it has too few children
				while(servers[i].children < servers[i].minProcs) {
				
					printf("Server %s has too few children. ", servers[i].name);
					printf("Spawning to compensate\n");
					
					//spawn one
					kill(servers[i].pid, SIGUSR1);
					pause();
					
					//increment the number of children it has
					servers[i].children++;
					
				}
				
				//while it has too many children
				while(servers[i].children > servers[i].maxProcs) {
				
					printf("Server %s has exceeded the child limit.", servers[i].name);
					printf(" Committing infanticide to compensate\n");
					
					//kill one
					kill(servers[i].pid, SIGUSR2);
					pause();
					
					//decrement the number of children it has
					servers[i].children--;
					
				}
			
			}
				
		}//end child range check
		
		printf("\n");
		
	} //end while that waits for commands

	return 0;

} //end main

