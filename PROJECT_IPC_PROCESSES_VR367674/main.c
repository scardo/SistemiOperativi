/*! \file main.c
*		\brief Main file of the project.
*/
#include "mylib.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <sys/sem.h>

#define MUTEX 0
//pointers for the shared memory
int *matA;
int *matB;
int *matC;
int *sum;
//IDs of the shared memory
int shmida;
int shmidb;
int shmidc;
int shmids;
//ID of the msg queue
int msgid;
//ID of the semaphore
int semid;
//2D pointer for the pipe
int **p;
//support variables
int i;
int j;
char buf[100];
char bufs[100];
//number of processes
int nproc;

/**
*@brief Signal handler of the master process in case of problems
*@param s An integer that indicates the number of the signal.
*
*This function detaches and cleans the shared memory, and cleans the semaphore before killing the process.
*Used only for problems.
*/
void onexit(int s){

	fdprint(1,"trying clean shared memory, semaphore, and message queue...\n");
	//detach the shared memory
	
	shmdt(matA);
	shmdt(matB); 
	shmdt(matC); 
	shmdt(sum); 
	
	//delete the shared memory

	shmctl(shmida,IPC_RMID,NULL);
	shmctl(shmidb,IPC_RMID,NULL);
	shmctl(shmidc,IPC_RMID,NULL);
	shmctl(shmids,IPC_RMID,NULL);

	//delete the message queue
	msgctl ( msgid , IPC_RMID , NULL );
	
		
	//delete the semaphore
	semctl(semid, 0, IPC_RMID, 0);


	strcpy(buf,"pid=");
	strcat(buf,my_itoa(getpid(),bufs,10));
	strcat(buf,"Master process is dead abnormally.\n");
	fdprint(1,buf);
	
	if(s==SIGCHLD){
		fdprint(2,"Abnormal exit of workers. Abort.\n");
	}
	
	
	if(s!=SIGKILL){
		signal(SIGINT, SIG_DFL);
		kill(getpid(),SIGINT);
	}
}

/**
*@brief Signal handler of the worker process
*@param s An integer that indicates the number of the signal.
*
*This function detaches the shared memory before killing the process.
*Used only for problems.
*/
void fdead(int s){
	//detach the shared memory
	
	shmdt(matA);
	shmdt(matB);
	shmdt(matC);
	shmdt(sum);

	strcpy(buf,"pid=");
	strcat(buf,my_itoa(getpid(),bufs,10));
	strcat(buf," worker is dead...\n");
	fdprint(1,buf);

	
	if(s!=SIGKILL){
		signal(SIGINT, SIG_DFL);
		kill(getpid(),SIGINT);
	}
	
}

/**
*@brief main function
*@param argc An integer argument count of the command line arguments.
*@param argv An argument vector of the command line arguments. 
*
*Command line arguments must be in the following format:\n
*\<matrixA path> \<matrixB path> \<matrixC path> \<matrix order> \<number of processes to use>
*/
int main (int argc, char **argv){

	if (argc != 6) {
		fdprint(2,"Specify five arguments.\n");
		kill(getpid(),SIGINT);
	}
	if (atoi(argv[4])<1 || atoi(argv[5])<1) {
		fdprint(2,"The order of a matrix and the number of processes must be greater then zero.\n");
		kill(getpid(),SIGINT);
	}


	
	//register signals handlers
	signal(SIGKILL,onexit);
	signal(SIGINT,onexit);
	signal(SIGTERM,onexit);
	signal(SIGHUP,onexit);
	signal(SIGCHLD,onexit);
	
	
	
	//number of process
	int nproc=atoi(argv[5]);
 
	//order of matrix
	int order=atoi(argv[4]);

	
	//vector with worker's pids
	int pidc[nproc];


	fdprint(1,"creating shared memory...\n");
	//create the shared memory
	if(
		(shmida=shmget(ftok("main.x",1), sizeof(int)*order*order,0666| IPC_CREAT))==-1 ||
		(shmidb=shmget(ftok("main.x",2), sizeof(int)*order*order,0666| IPC_CREAT))==-1 ||
		(shmidc=shmget(ftok("main.x",3), sizeof(int)*order*order,0666| IPC_CREAT))==-1 ||
		(shmids=shmget(ftok("main.x",4), sizeof(int) ,0666| IPC_CREAT))==-1

	){
		syserr("main", "Cannot create the shared memory.");
	}
	//attach the shared memory
	if(
		(matA=shmat(shmida,NULL,0))<0 ||
		(matB=shmat(shmidb,NULL,0))<0 ||
		(matC=shmat(shmidc,NULL,0))<0 ||
		(sum =shmat(shmids,NULL ,0))<0
	){
		syserr("main", "Cannot attach the shared memory.");
	}
	*sum=0;


	fdprint(1,"creating mutex...\n");
	//create the semaphore
	if((semid=semget(ftok("main.x",5), 1 , 0666|IPC_CREAT|IPC_EXCL ))==-1){
		syserr("main", "Cannot create the mutex.");
	}
	union semun {                              
    int val;
    struct semid_ds *buf;
    ushort *array;
  	} st_sem;

  //initialize the semaphore to 1
	st_sem.val = 1;
  if (semctl(semid, MUTEX, SETVAL, st_sem) == -1) {
  	syserr("main", "Cannot initialize the mutex.");
  }






	fdprint(1,"creating pipe...\n");
	//create dinamically the pipe
	p=(int**)malloc(nproc*sizeof(int*));
	for(i=0; i<nproc;i++){
    p[i]=(int*)malloc(2*sizeof(int));


   	if(pipe(p[i])==-1){
   		syserr("main", "Cannot create the pipe.");
		}
	}

  //create the struct for msgs of the message queue
 	struct message {

  	long type;
  	int pindex;
 
      
	}msg;

	//create the struct of the message for the pipe
	struct operation {
  	int code;
  	int row;
  	int column;
      
	}op;

	fdprint(1,"creating the message queue...\n");
	
	//create the message queue
	if((msgid=msgget(ftok("main.x",6),0666|IPC_CREAT|IPC_EXCL))==-1){
		syserr("main", "Cannot create the message queue.");
	}

	//create the workers
	fdprint(1,"creating worker processes...\n");
	for (i = 0; i < nproc; i++) {
  	if ((pidc[i] = fork()) < 0) {
  		syserr("main", "fork() failure.");   	
  	} else if (pidc[i] == 0) {
    	//NB now i becomes an in-application process identifier
    	//<child code>

   
  		//register signal handlers
    	signal(SIGKILL,fdead);
			signal(SIGINT,fdead);
			signal(SIGTERM,fdead);
    	signal(SIGHUP,fdead);
  		prctl(PR_SET_PDEATHSIG, SIGHUP); //ask to the kernel to send a SIGHUP when the worker die
  		
  		//close the writing side of the pipe
  		if(close(p[i][1])==-1){
  			syserr("main", "Cannot close pipe[process][1].");
  		}

    	
  		while(1==1){
  			int n;
    		msg.type=1; //ready code
  			msg.pindex=i;
 

  			//send the ready message
  			if(msgsnd(msgid, &msg, sizeof(msg), 0)==-1){
					syserr("main", "Cannot send messages on the message queue.");

				}

				
				
				//waiting operations
				if(read(p[i][0], &op, sizeof(op))==-1){
					syserr("main", "Cannot read pipe[process][0].");
				}
				

				switch(op.code){
					case 0:
						//multiplication operation recieved
						multcalc(order,matA,matB,matC,op.row,op.column);

						msg.type=2; //done message
  					msg.pindex=i;
  					

  					//send the done message
  					if(msgsnd(msgid, &msg, sizeof(msg), 0)==-1){
							syserr("main", "Cannot send messages on the message queue.");
						}
						break;
					
					case 1:
						//sum operation recieved
						n=sumrow(order,matC,op.row); //do the sum row
						
						sem_wait(semid,MUTEX);
						//<critical zone>
					
						*sum+=n; //add the calculated sum in the shared memory


						//</critical zone>
						sem_signal(semid,MUTEX);
						msg.type=3; //sum done code
  					msg.pindex=i;
  					

  					//send the done code
  					if(msgsnd(msgid, &msg, sizeof(msg), 0)==-1){
							syserr("main", "Cannot send messages on the message queue.");

						}
						
						
						break;
					case 2:
						//recieved the code for killing the worker
						msg.type=4; //done code
  					msg.pindex=i;
  					//detach the memory
  					if(
							shmdt(matA)==-1 ||
							shmdt(matB)==-1 ||
							shmdt(matC)==-1 ||
							shmdt(sum)==-1
						){
							syserr("main", "Worker's shared memory detaching failure.");
						}

						strcpy(buf,"pid=");
						strcat(buf,my_itoa(getpid(),bufs,10));
						strcat(buf," worker is dead...\n");
						fdprint(1,buf);

  					//send the message to inform the master that the worker is going to exit
  					if(msgsnd(msgid, &msg, sizeof(msg), 0)==-1){
							syserr("main", "Cannot send messages on the message queue.");

						}
						exit(0);


						break;
					default:						
						break;
				}

  		}
    	
    	//</child code>
  	}
	} 
	//<father code>


	//close all reading sides of the pipe
	for (i=0; i<nproc; i++){
		if(close(p[i][0])==-1){
			syserr("main", "Cannot close pipe[process][0].");
		}
	}
	
	//load matrix in the shared memory
	fdprint(1,"loading the first matrix...\n");
	matin(order, matA, argv[1]);
	fdprint(1,"loading the second matrix...\n");
	matin(order, matB, argv[2]);
	
	


	fdprint(1,"calculating the multiplication...\n");
	//for each element of the result matrix
	for(i=0; i<order; i++){
		for(j=0; j<order; j++){
			//wait a ready worker
			if((msgrcv(msgid, &msg, sizeof(msg), 1, 0  ))==-1){
				syserr("main", "Cannot recieve messages from the messages queue.");

			}
			op.code=0;
			op.row=i;
			op.column=j;
			//send to the waiting worker the element to calculate
			if(write(p[msg.pindex][1],&op,sizeof(op))==-1){
				syserr("main", "pipe writing failure.");
			}

		}
	}

	//checking if master has recieved all the results
	for(i=0; i<order*order; i++){
		if((msgrcv(msgid, &msg, sizeof(msg), 2, 0  ))==-1){
				syserr("main", "Cannot recieve messages from the messages queue.");

		}
	}


	fdprint(1,"writing the result in file...\n");
	
	//write the result matrix in the file
	matout(order,matC,argv[3]);

	
	fdprint(1,"calculating the sum...\n");
	//for each row
	for(j=0; j<order; j++){
		//get the ready worker
		if((msgrcv(msgid, &msg, sizeof(msg), 1, 0  ))==-1){
				syserr("main", "Cannot recieve messages from the messages queue.");

		}
		op.code=1;
		op.row=j;
		op.column=-1;
		//send to the ready worker the index for calculating the sum of the row
		if(write(p[msg.pindex][1],&op,sizeof(op))==-1){
			syserr("main", "pipe writing failure.");
		}

	}
	//checking if the sum is done
	for(j=0;j<order;j++){
		if((msgrcv(msgid, &msg, sizeof(msg), 3, 0  ))==-1){
				syserr("main", "Cannot recieve messages from the messages queue.");

		}
	}

	//print the value sum to the standard output
	strcpy(buf,"value of the sum: ");
	strcat(buf,my_itoa(*sum,bufs,10));
	strcat(buf,"\n");
	fdprint(1,buf);


	fdprint(1,"killing workers...\n");
	//sending the command to terminate the processes
	signal(SIGCHLD, SIG_DFL);
	for(j=0;j<nproc;j++){
		op.code=2;
		op.row=-1;
		op.column=-1;
		if(write(p[j][1],&op,sizeof(op))==-1){
			syserr("main", "pipe writing failure.");
		}

	}
	//checking if the work is done
	
	for(j=0;j<nproc;j++){
		if((msgrcv(msgid, &msg, sizeof(msg), 4, 0  ))==-1){
				syserr("main", "Cannot recieve messages from the messages queue.");

		}
	}
	
	//starting to kill the master process		
	fdprint(1,"cleaning shared memory...\n");
	//detach the shared memory
	if(
		shmdt(matA)==-1 ||
		shmdt(matB)==-1 ||
		shmdt(matC)==-1 ||
		shmdt(sum)==-1 
	){
		syserr("main", "Master's shared memory detaching failure.");
	}
	//delete the shared memory
	if(
		shmctl(shmida,IPC_RMID,NULL)==-1 ||
		shmctl(shmidb,IPC_RMID,NULL)==-1 ||
		shmctl(shmidc,IPC_RMID,NULL)==-1 ||
		shmctl(shmids,IPC_RMID,NULL)==-1 
	){
		syserr("main", "Master's shared memory removing failure.");
	}
	fdprint(1,"cleaning message queue...\n");
	//delete the message queue
	if(msgctl ( msgid , IPC_RMID , NULL )==-1){
		syserr("main", "Master's messages queue removing failure.");
	}
		
	fdprint(1,"cleaning semaphore...\n");
	//delete the semaphore
	if(semctl(semid, 0, IPC_RMID, 0)==-1){
		syserr("main", "Master's semaphore removing failure.");
	}

	fdprint(1,"freeing the pipe...\n");

	//free the pipe
	for(i=0; i<nproc;i++){
    free(p[i]);
  }
	free(p);


	fdprint(1,"killing master...\n");

	exit(0);

	//</father code>	
}


