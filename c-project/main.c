
/*****************************************
*   AaP - semestralna praca - "Skicar"   *
*                  -                     *
*          FILIP KALUS - 3Z1A10          *
*                  -                     *
*                 2018                   *
*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <windows.h>
#include <conio.h>
#include <string.h>
#include <stdbool.h>
#include "funkcie.h"

// Ascii values of symbols used to draw the frame of drawing area
#define VT 186
#define DL 187
#define HL 188
#define HP 200
#define DP 201
#define HZ 205

// Keycodes
enum{
    KEY_ESC     = 27,
    ARROW_UP    = 256 + 72,
    ARROW_DOWN  = 256 + 80,
    ARROW_LEFT  = 256 + 75,
    ARROW_RIGHT = 256 + 77
};

// Variables
char dataFile[32] = "xy.csv";   // File to save/load the drawing
char znak = '#';                // Character used to draw in drawing area

void setup(),menu(),vytvor(),nacitaj();
int ch;                         // Variable used in get_code() function
int q;                          // Counter for indexing an array
int cursorPosX,cursorPosY;      // X,Y coords of cursor
int moznostMenu=1;              // Current option in menu => oznacMoznost()
char c;                         // Return value from getCursorChar()
int saveData[2][2100];          // 2D array used to store X,Y coords of cursor
int rectX,rectY;                // X,Y coords used to draw a rectangle
bool rectActive = false;        // Used to draw a rectangle

int main(){
    setup();
z1: menu();
    clearArea();
    switch(moznostMenu){
        case 1:
            vytvor();
        case 2:
            nacitaj();
            vytvor();
        case 3:
            klavesy();
            clearArea();
            moznostMenu=1;
            goto z1;
        case 4:
            gotoxy(0,32);
            // Killing the app
            system("taskkill/IM cb_console_runner.exe");
        default:
            break;
    }
    return 0;
}

void vytvor(){
    cursorPosX=2;
    cursorPosY=29;
    gotoxy(0,31);
    printf(" X= %d   \n Y= %d   ",cursorPosX-2,cursorPosY-2);
    gotoxy(cursorPosX,cursorPosY);
    while((ch = getCode()) != KEY_ESC){
        switch(ch){
            // MOVE UP
            case ARROW_UP:
                if(cursorPosY>2){ // upper border
                    cursorPosY--;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            // MOVE DOWN
            case ARROW_DOWN:
                if(cursorPosY<29){ // lower border
                    cursorPosY++;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            // MOVE LEFT
            case ARROW_LEFT:
                if(cursorPosX>2){ // left border
                    cursorPosX--;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            // MOVE RIGHT
            case ARROW_RIGHT:
                if(cursorPosX<69){ // right border
                    cursorPosX++;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            // DRAWING A CHARACTER INTO DRAWING AREA
            case ' ': // IF SPACE IS PRESSED
                printf("%c",znak);
                gotoxy(cursorPosX,cursorPosY);
                break;
            // CREATING RECTANGLE
            case 'r':
                if(!rectActive){
                    rectX=cursorPosX;
                    rectY=cursorPosY;
                    printf("R");
                    gotoxy(cursorPosX,cursorPosY);
                } else {
                    if(rectX>cursorPosX){
                        if(rectY>cursorPosY){
                            for(int i=cursorPosX;i<=rectX;i++){
                                for(int j=cursorPosY;j<=rectY;j++){
                                    gotoxy(i,j);
                                    printf("%c",znak);
                                }
                            }
                        } else {
                            for(int i=cursorPosX;i<=rectX;i++){
                                for(int j=rectY;j<=cursorPosY;j++){
                                    gotoxy(i,j);
                                    printf("%c",znak);
                                }
                            }
                        }
                    } else {
                        if(rectY>cursorPosY){
                            for(int i=rectX;i<=cursorPosX;i++){
                                for(int j=cursorPosY;j<=rectY;j++){
                                    gotoxy(i,j);
                                    printf("%c",znak);
                                }
                            }
                        } else {
                            for(int i=rectX;i<=cursorPosX;i++){
                                for(int j=rectY;j<=cursorPosY;j++){
                                    gotoxy(i,j);
                                    printf("%c",znak);
                                }
                            }
                        }
                    }
                }
                rectActive=!rectActive;
                break;
            // DELETING CHARACTER ON CURSOR POSITION
            case 'c':
                printf(" ");
                gotoxy(cursorPosX,cursorPosY);
                break;
            // ERASING WHOLE DRAWING AREA
            case 'm':
                clearArea();
                gotoxy(cursorPosX,cursorPosY);
                break;
            // SHOWING CONTROLS
            case 'k':
                q = 0;
                // Saving the drawing area before erasing it
                for(int i=2;i<70;i++){
                    for(int j=2;j<30;j++){
                        gotoxy(i,j);
                        if(getCursorChar()==znak){
                            saveData[0][q]=i;
                            saveData[1][q]=j;
                            q++;
                        }
                    }
                }
                clearArea();
                // Showing actual controls
                klavesy();
                clearArea();
                // Loading saved image back to drawing area
                for(int j=0;j<2100;j++){
                    for(int k=2;k<70;k++){
                        for(int l=2;l<30;l++){
                            if(k==saveData[0][j]&&l==saveData[1][j]){
                                gotoxy(k,l);
                                printf("%c",znak);
                            }
                        }
                    }
                }
                gotoxy(cursorPosX,cursorPosY);
                clearSaveData();
                break;
            // SAVING DRAWED PICTURE TO FILE
            case 's':
                // Storing X,Y coords to 2D array
                q = 0;
                for(int i=2;i<70;i++){
                    for(int j=2;j<30;j++){
                        gotoxy(i,j);
                        if(getCursorChar()==znak){
                            saveData[0][q]=i;
                            saveData[1][q]=j;
                            gotoxy(1,33);
                            printf("Ukladanie znaku na adresy: %x-%x       ",&saveData[0][q],&saveData[1][q]);
                            q++;
                            //Sleep(50); //TEST VYPISOVANIA ADRESY
                        }
                    }
                }
                // Copying X,Y coords from array to file
                FILE *f = fopen(dataFile, "wb");
                for(int i=0;i<2100;i++){
                    for(int j=0;j<2;j++){
                        if(!saveData[1][i]==0) fprintf(f,"%02d,",saveData[j][i]);
                    }
                    if(!saveData[1][i]==0) fprintf(f,"\n");
                }
                fclose(f);
                gotoxy(1,33);
                printf("Ukladanie dokoncene...");
                printf("                                 ");
                break;
            default:
                break;
            }
        gotoxy(0,31);
        // Showing current cursor position
        printf(" X= %d   \n Y= %d   ",cursorPosX-2,cursorPosY-2);
        gotoxy(cursorPosX,cursorPosY);
    }
    gotoxy(0,32);
    // Killing the app
    system("taskkill/IM cb_console_runner.exe");
}

void nacitaj(){
    gotoxy(1,33);
    printf("Prebieha nacitavanie...");
    // Loading X,Y coords from file to 2D array
    getFileData(dataFile);
    // Drawing characters from array to drawing area
    for(int j=0;j<2100;j++){
        for(int k=2;k<70;k++){
            for(int l=2;l<30;l++){
                if(k==saveData[0][j]&&l==saveData[1][j]){
                    gotoxy(k,l);
                    printf("%c",znak);
                }
            }
        }
    }
    clearSaveData();
}

void menu(){
    bool inMenu = true;
    gotoxy(2,29);
    printf("FILIP KALUS 2018");
    gotoxy(32,11);
    printf("ZALOZIT");
    gotoxy(32,14);
    printf("NACITAT");
    gotoxy(32,17);
    printf("KLAVESY");
    gotoxy(32,20);
    printf("UKONCIT");
    gotoxy(30,11);
    printf(">");
    gotoxy(40,11);
    printf("<");
    // Menu controls
    while((ch = getCode()) != KEY_ESC){
	    switch(ch){
            case ARROW_UP:
                if(moznostMenu!=1) moznostMenu--;
                break;
            case ARROW_DOWN:
                if(moznostMenu!=4) moznostMenu++;
                break;
            case ' ':
                inMenu = false;
                break;
	    }
	    oznacMoznost(moznostMenu);
	    if(!inMenu) break;
    }
    gotoxy(0,32);
}

void setup(){
    // Changing the name of app
    SetConsoleTitle("Skic\xA0r"); // \xA0 = á
    // Clearing the 2D array
    clearSaveData();
    // Frame of the drawing area
    gotoxy(1,1);
    printf("%c",DP);
    for(int i=0;i<68;i++) printf("%c",HZ);
    printf("%c\n",DL);
    for(int i=0;i<28;i++){
        printf(" %c",VT);
        for(int j=0;j<68;j++) printf(" ");
        printf("%c\n",VT);
    }
    printf(" %c",HP);
    for(int i=0;i<68;i++) printf("%c",HZ);
    printf("%c",HL);
}
