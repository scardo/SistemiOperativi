/*! \file mylib.c
*   \brief Library file with the support functions.    
*/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <signal.h>
#include <errno.h>
#include <sys/sem.h>
#include "mylib.h"
#ifndef SIZE
#define SIZE 512 /** dimension of the buffer */
#endif

/**
*@brief Print a string on an output pointed by a file descriptor.
*@param fd The integer that indicates the file descriptor.
*@param msg The char array string.
*@return The length of the printed string.
*/
int fdprint (int fd, char msg[]){
	//calculate the length of the given string 
  int length=strlen(msg); 
  

  //for each char of the string, print it on the output pointed by the given file descriptor
	if(write(fd,msg,length)==-1){
		syserr("main", "fdprint write() failure.");
	}
  
  //return the length of the string
	return length;

}

/**
*@brief Load a matrix from a file in the shared memory.
*@param order The integer that indicates the order of the matrix.
*@param matrix Integer pointer of the shared memory.
*@param path char array that indicates the matrix path file to load.
*@return The order of the matrix loaded.
*
*All the elements of the matrix in the file must have a ";" as separator, without spaces,and all in the same line.
*/
int matin (int order, int *matrix,  char path[]){
	int fd;
	int r=0; //row
	int c=0; //column
  int n;
	char buf[SIZE];

	char * pch; //char pointer used for parsing the input matrix

  //open the stream and get the file descriptor
	if ((fd = open(path,O_RDONLY,0644)) == -1) {
			syserr("main", "matin open() failure.");
	}

  //read the matrix and save it in the buffer
	if ( (n=read(fd,buf,SIZE)) > 0){
	  	
    //parse the content of the buffer
  	pch = strtok (buf,";");
  	while (pch != NULL){
    		
      //store the parsed values in the shared memory
  		matrix[r*sizeof(int)+c]=atoi(pch);
  		  	
    	pch = strtok (NULL, ";");
    	c++;
    	if(c==order){

    		c=0;
    		r++;
    	}

  	}


		
	}else if(n==-1){
    syserr("main", "matin read() failure.");
  }

  //close the stream
	if(close(fd)==-1){
		syserr("main", "matin close() failure.");
	}

	return order;
}

/**
*@brief Store a matrix in a file from the shared memory.
*@param order The integer that indicates the order of the matrix.
*@param matrix Integer pointer of the shared memory.
*@param path char array that indicates the file path to store.
*@return The order of the matrix stored.
*
*The stored matrix will be in the following format:\n
*e1;e2;e3;...
*/
int matout(int order, int *matrix,  char path[]){
	int fd;
	int r;
	int c;
	char buf[SIZE];

	
  //open the stream
	if ((fd = open(path, O_RDWR | O_TRUNC,0644)) == -1) {
			syserr("main", "matout open() failure.");
	}
  		
  //for each element of the matrix
  for(r=0; r<order; r++){
  	for(c=0; c<order; c++){
  		
  		
  		//get the integer, parse it as char, and write it to the file descriptor
  		my_itoa(matrix[r*sizeof(int)+c],buf,10) ;
  		fdprint(fd,buf );
  		
      //add a semicolon, except for the last element
  		if(r!=(order-1) || c!=(order-1)){
  			fdprint(fd,";" );

  		}
	
  	}


  }
    
  //close the stream
	if(close(fd)==-1){
		syserr("main", "matout close() failure.");
	}

	return order;
}

/**
*@brief Matrix multiplication
*@param order The integer that indicates the order of the matrix.
*@param matrixA Integer pointer of the shared memory with the matrix A.
*@param matrixB Integer pointer of the shared memory with the matrix B.
*@param matrixC Integer pointer of the shared memory with the matrix C.
*@param rC Integer row index of the element to calculate.
*@param cC Integer column index of the element to calculate.
*
*This function calculates the element with coordinates (rC,cC) of the multiplication between matrixA and matrixB,
*and stores the result in matrixC.
*/
void multcalc(int order, int *matrixA, int *matrixB, int *matrixC, int rC, int cC){

  //initialize with 0 the (rC,cC) element of the result matrix in the shared memory
  matrixC[rC*sizeof(int)+cC]=0;
  int i;

  for(i=0; i<order; i++){
    
    //do the row-column multiplication
    matrixC[rC*sizeof(int)+cC]+=(matrixA[rC*sizeof(int)+i])*(matrixB[i*sizeof(int)+cC]);
      
  }
  

}

/**
*@brief Sum of the elements of a matrix row.
*@param order The integer that indicates the order of the matrix.
*@param matrixC Integer pointer of the shared memory with the matrix C.
*@param rC Integer row index.
*@return Integer sum result.
*/
int sumrow(int order,int *matrixC,int rC){


  int i;
  int s=0;


  for(i=0; i<order; i++){
    //sum all elements of the same row
    s+=matrixC[rC*sizeof(int)+i];
      
  }

  return s;

}

/**
*@brief Reverse of a string.
*@param str String you have to reverse.
*@param len Integer string length.
*/
void my_reverse(char str[], int len){
    int start, end;
    char temp;
    //swap all elements of the string starting with the first and the last char.
    //for each iteration, reduce the field to do the swap
    for(start=0, end=len-1; start < end; start++, end--) {
        temp = *(str+start);
        *(str+start) = *(str+end);
        *(str+end) = temp;
    }
}
 
/**
*@brief Integer to char array.
*@param num Integer to convert.
*@param str Pointer of the string.
*@param base Integer base of the number to convert.
*@return str Pointer of the converted string.
*
*Convert an integer into a char array. 
*Negative numbers are only handled if base is 10 otherwise considered unsigned number.
*/ 
char* my_itoa(int num, char* str, int base){
    int i = 0; //index for the result string
    int isNegative = 0;
  	
    //the implest case. A zero is same "0" string in all base
    if (num == 0) {
        str[i] = '0';
        str[i + 1] = '\0';
        return str;
    }
  
    //negative numbers are only handled if base is 10 
    //otherwise considered unsigned number 
    if (num < 0 && base == 10) {
        isNegative = 1;
        num = -num;
    }
    
    while (num != 0) {
        int rem = num % base; //get the digit to convert
        str[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0'; //convert the digit to the correct char
        num = num/base; //refresh the number to convert to get the next digit
    }
  
    // Append negative sign for negative numbers
    if (isNegative==1){
        str[i++] = '-';
    }
  
    //"close" the string
    str[i] = '\0';
 
    //reverse the string
    my_reverse(str, i);
  
    return str;
}

/**
*@brief Implementation of P() of semaphores.
*@param semid The integer ID of the semaphore array.
*@param sem_number An integer that indicates the index of the semaphore array.
*/
void sem_wait(int semid, int sem_number) {

  //create the struct to control the semaphore
  struct sembuf wait_b;

  wait_b.sem_num = sem_number;   
  wait_b.sem_op = -1; //decrement the semaphore                    
  wait_b.sem_flg = 0;                    

  if (semop(semid, &wait_b, 1) == -1) {     
    syserr("main", "Cannot use the mutex.");
  }
}

/**
*@brief Implementation of V() of semaphores.
*@param semid The integer ID of the semaphore array.
*@param sem_number An integer that indicates the index of the semaphore array.
*/
void sem_signal(int semid, int sem_number) {

  //create the struct to control the semaphore
  struct sembuf signal_b;
  signal_b.sem_num = sem_number; 
  signal_b.sem_op = 1; //increment the semaphore                    
  signal_b.sem_flg = 0;                      

  if (semop(semid, &signal_b, 1) == -1) {
    syserr("main", "Cannot use the mutex.");
  }
}


/**
*@brief Report system errors.
*@param prog char array with the name of the program.
*@param msg message to print
*
*This function reports info about the last system error.
*After that, a SIGINT signal is sent to the process that have called the function.
*/
void syserr (char *prog, char *msg){
  char buf[SIZE];

  
  strcpy(buf,prog);
  strcat(buf," - error: ");
  strcat(buf,msg);
  strcat(buf,"\nproc");
  fdprint(2,buf);
  fdprint(2,"system error\n");
  

  kill(getpid(),SIGINT);
}