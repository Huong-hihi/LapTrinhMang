#ifndef __TOOL__
#define __TOOL__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

//Chia nhỏ message
void splitMessage(char *input, char *code, char *data);
void splitMessageData(char *input, char *data1, char *data2);
void splitQuestionFromFile(char *input, char *stt, char *level, char *question, char *answer1, char *answer2, char *answer3, char *answer4, char *answerTrue);
void splitHelpFromFile(char *input, char *stt, char *key, char *value);
//random số trong mảng
int randomNumberInArray(int *numberArray, int length);

#endif