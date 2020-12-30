#ifndef __VALID__
#define __VALID__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFF_SIZE 2014

#define TRUE 1
#define FALSE 0

int checkValue(int value);                    //Kiểm tra giá trị 
int checkDigit(char character);               //Kiểm tra số
int checkDotInString(char *string);           //Kiểm tra dot
int checkIP(char *string);                    //Kiểm tra IP
int checkPort(char *port);                    //Kiểm tra cổng
int checkSpace(char *input);                  //Kiểm tra khoảng trống
int checkIPAndPort(char *ipaddr, char *port); //Kiểm tra  IP và dot
#endif