#ifndef FUNKCIE_H_INCLUDED
#define FUNKCIE_H_INCLUDED

#include <string.h>

int getCode();
char getCursorChar();
void gotoxy(int x, int y);
void clearArea();
void clearSaveData();
void oznacMoznost(int g);
void klavesy();
void getFileData(char fileName[32]);

#endif // FUNKCIE_H_INCLUDED
