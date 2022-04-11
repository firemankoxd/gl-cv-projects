#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <string.h>

int saveData[2][2100];

// Getting keycode
int getCode (void){
    int ch = getch();
    if(ch==0 || ch==224)
        ch = 256 + getch();
    return ch;
}

// Getting char based on a cursor position
char getCursorChar(){
    char c = '\0';
    CONSOLE_SCREEN_BUFFER_INFO con;
    HANDLE hcon = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hcon != INVALID_HANDLE_VALUE &&
        GetConsoleScreenBufferInfo(hcon,&con))
    {
        DWORD read = 0;
        if (!ReadConsoleOutputCharacterA(hcon,&c,1, con.dwCursorPosition,&read) || read != 1)
            c = '\0';
    }
    return c;
}

// Moving cursor in the console
void gotoxy(int x, int y){
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Clearing all characters on drawing area
void clearArea(){
    gotoxy(2,2);            // jump to the start of drawing area
    for(int i=2;i<30;i++){
        gotoxy(2,i);        // jump to the start of i-th row
        for(int j=2;j<70;j++){
            printf(" ");
        }
        printf("\n");
    }
}

// Clearing the savedata array
void clearSaveData(){
    for(int j=0;j<2;j++){
        for(int i=0;i<2100;i++){
            saveData[j][i]=0;
        }
    }
}

// Highligting options in menu
void oznacMoznost(int g){
    switch(g){
        case 1:
            gotoxy(30,14);
            printf(" ");
            gotoxy(40,14);
            printf(" ");
            gotoxy(30,11);
            printf(">");
            gotoxy(40,11);
            printf("<");
            break;
        case 2:
            gotoxy(30,11);
            printf(" ");
            gotoxy(40,11);
            printf(" ");
            gotoxy(30,17);
            printf(" ");
            gotoxy(40,17);
            printf(" ");
            gotoxy(30,14);
            printf(">");
            gotoxy(40,14);
            printf("<");
            break;
        case 3:
            gotoxy(30,20);
            printf(" ");
            gotoxy(40,20);
            printf(" ");
            gotoxy(30,14);
            printf(" ");
            gotoxy(40,14);
            printf(" ");
            gotoxy(30,17);
            printf(">");
            gotoxy(40,17);
            printf("<");
            break;
        case 4:
            gotoxy(30,17);
            printf(" ");
            gotoxy(40,17);
            printf(" ");
            gotoxy(30,20);
            printf(">");
            gotoxy(40,20);
            printf("<");
            break;
    }
}

// Listing all controls
void klavesy(){
    gotoxy(10,10);
    printf("Pohyb:\t  SIPKY");
    gotoxy(10,12);
    printf("Kreslenie:\t  MEDZERNIK");
    gotoxy(10,14);
    printf("Obdlznik:\t  R (2x)");
    gotoxy(10,16);
    printf("Mazanie znaku:  C");
    gotoxy(10,18);
    printf("Mazanie plochy: M");
    gotoxy(10,20);
    printf("Ulozenie:\t  S");
    gotoxy(10,22);
    printf("Klavesy:\t  K");
    Sleep(5000);
}

// Loading data from csv file to 2D array
void getFileData(char fileName[32]){
    clearSaveData();
    int p,n;
    int b=0,v=0,countN=0;
    FILE *f = fopen(fileName, "r");
    if(f){
        do{
            p=fgetc(f);                     // Loading one character
            if(p==','){                     // Skip if char is separator (comma)
                countN = 0;
            } else if(p=='\n'){             // Skip is char is end of line
                countN = 0;
            } else if(p!=',' && p!='\n'){   // Continue if char is a number
                if(countN==0){
                    n = p - '0';            // ASCII value conversion to actual integer value
                    countN++;
                } else {
                    // If number has two digits (greater than 9)
                    int zvys = p - '0';
                    n = n*10 + zvys;
                    saveData[b][v]=n;
                    // Setting array indexes
                    if(b==0){
                        b++;
                    } else {
                        b--;
                        v++;
                    }
                }
            }
        //DO-WHILE till the end of the file
        } while (p!= EOF);
    gotoxy(1,33);
    printf("Nacitavanie dokoncene...");
    fclose(f);
    } else {
        gotoxy(1,33);
        printf("Nacitavanie neuspesne..");
    }
}
