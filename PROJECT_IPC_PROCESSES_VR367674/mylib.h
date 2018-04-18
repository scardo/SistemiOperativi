/** @file
*   @brief Definitions for mylib.c library.    
*/
#ifndef MYLIB_H
#define MYLIB_H


int fdprint (int fd, char msg[]);

int matin (int order, int *matrix,  char path[]);

int matout (int order, int *matrix,  char path[]);

void multcalc(int order, int *matrixA, int *matrixB, int *matrixC, int rC, int cC);

int sumrow(int order,int *matrixC,int rC);

void my_reverse(char str[], int len);
char* my_itoa(int num, char* str, int base);

void sem_wait(int semid, int sem_number);

void sem_signal(int semid, int sem_number);

void syserr (char *prog, char *msg);
#endif
