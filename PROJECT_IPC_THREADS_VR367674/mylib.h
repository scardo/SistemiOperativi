/** @file
*   @brief Definitions for mylib.c library.    
*/
#ifndef MYLIB_H
#define MYLIB_H

int fdprint (int fd, char msg[]);

int matin (int order, int *matrix,  char path[]);

int matout (int order, int *matrix,  char path[]);

int sumrow(int order,int *matrixC,int rC);

void my_reverse(char str[], int len);
char* my_itoa(int num, char* str, int base);

void syserr (char *prog, char *msg);

#endif
