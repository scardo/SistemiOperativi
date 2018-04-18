/*! \file main.c
*		\brief Main file of the project.
*/
#include "mylib.h"
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

//pointers for the shared memory
int *matA;
int *matB;
int *matC;
int *sum;

/**
*\brief struct used to pass parameters (row, colum) to threads
*/
struct readThreadParams{
	int rC; 
	int cC; 
};

//mutex used to prevent the race condition
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//buffers
char buf[100];
char bufs[100];
//number of threads
int nthr;

//pointer used to create the array of tids
pthread_t *tidc;

//order of the matrix
int order;



/**
*@brief Thread for the matrix multiplication
*@param context pointer of a struct with integer coordinates (rC,cC)
*
*This function calculates the element with coordinates (rC,cC) of the multiplication between matrixA and matrixB,
*and stores the result in matrixC.
*/
void* multcalc(void *context){
	
	//takes params
	struct readThreadParams *rParams = context;
  


  int i;

  for(i=0; i<order; i++){
    
    //do the row-column multiplication
    matC[(*rParams).rC*sizeof(int)+(*rParams).cC]+=(matA[(*rParams).rC*sizeof(int)+i])*(matB[i*sizeof(int)+(*rParams).cC]);
    
    
  }
  free(context);
  pthread_exit(NULL);
  

}

/**
*@brief Thread used to calculate the sum of each element of the matrixC
*@param context pointer of a struct with the index of the row to calculate
*
*/
void* dosum(void *context){
	
	//take params
	struct readThreadParams *rParams = context;
	
	//do the sum of the row
	int n=sumrow(order,matC,(*rParams).rC); 
            
  //P()
  pthread_mutex_lock(&mutex);
        
  //<critical zone>
          
  *sum+=n; //add the calculated sum in the shared memory


  //</critical zone>
  //V()
  pthread_mutex_unlock(&mutex);
  
  free(context);
  pthread_exit(NULL);
}


/**
*@brief main function
*@param argc An integer argument count of the command line arguments.
*@param argv An argument vector of the command line arguments. 
*
*Command line arguments must be in the following format:\n
*\<matrixA path> \<matrixB path> \<matrixC path> \<matrix order> \<number of threads to use>
*/
int main (int argc, char **argv){

	//support variables
	int i;
	int j;



	if (argc != 5) {
		fdprint(2,"Specify four arguments.\n");
		exit(1);
	}
	if (atoi(argv[4])<1 ) {
		fdprint(2,"The order of a matrix must be greater then zero.\n");
		exit(1);
	}


	
	
	
 
	//order of matrix
	order=atoi(argv[4]);

	//number of thread
	int nthr=order;


	//create the space for the array with tids
	tidc=malloc(sizeof(pthread_t)*nthr);


	fdprint(1,"creating shared memory...\n");
	//create the shared memory
	matA=(int*)malloc(sizeof(int)*order*order);
	matB=(int*)malloc(sizeof(int)*order*order);
	matC=(int*)calloc(order*order, sizeof(int));
	sum=(int*)calloc(1, sizeof(int));



	//load matrix in the shared memory
	fdprint(1,"loading the first matrix...\n");
	matin(order, matA, argv[1]);
	fdprint(1,"loading the second matrix...\n");
	matin(order, matB, argv[2]);

	
	


	//calculating matC
	fdprint(1,"calculating matC using threads...\n");
	//for each line
	for (i = 0; i < nthr; i++) {

		for (j=0; j<nthr; j++){
			
			//prepare params for the threads
			struct readThreadParams *readParams;
			readParams=malloc(sizeof(int)*2);
			(*readParams).rC=i;
			(*readParams).cC=j;

			//creating threads
			if (pthread_create( &(tidc[j]) ,NULL, &multcalc, readParams) < 0) {
				
  			syserr("main", "thread failure.");   	
  		} 
		}

		//wait threads terminations
		for(j=0; j<nthr;j++){
			if(pthread_join(tidc[j], NULL)!=0){
				syserr("main", "thread join failure.");
			}
		}

	}



	fdprint(1,"writing the result in file...\n");
	
	//write the result matrix in the file
	matout(order,matC,argv[3]);

	
	fdprint(1,"calculating the sum...\n");
	//for each row
	for(j=0; j<order; j++){

			//prepare params
			struct readThreadParams *readParams;
			readParams=malloc(sizeof(int)*2);
			(*readParams).rC=j;
			(*readParams).cC=-1;

			//create threads
			if (pthread_create( &(tidc[j]) ,NULL, &dosum, readParams) < 0) {
  			syserr("main", "thread failure.");   	
  		} 
	}
	
	//wait threads terminations
	for(j=0; j<nthr;j++){
			pthread_join(tidc[j], NULL);
	}

	//print the value sum to the standard output
	strcpy(buf,"value of the sum: ");
	strcat(buf,my_itoa(*sum,bufs,10));
	strcat(buf,"\n");
	fdprint(1,buf);


	
		
	fdprint(1,"cleaning memory...\n");
	
	free(matA);
	free(matB);
	free(matC);
	free(sum);

	free(tidc);

	pthread_mutex_destroy(&mutex);

	fdprint(1,"killing master...\n");

	exit(0);


}


