
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

//ASCII KODY SYMBOLOV POUZITYCH NA TVORBU PROSTREDIA
#define VT 186
#define DL 187
#define HL 188
#define HP 200
#define DP 201
#define HZ 205

//sucast kodu pre overenie stlacenej klavesy
enum{
    KEY_ESC     = 27,
    ARROW_UP    = 256 + 72,
    ARROW_DOWN  = 256 + 80,
    ARROW_LEFT  = 256 + 75,
    ARROW_RIGHT = 256 + 77
};

/*--------NASTAVENIA--------*/
char dataFile[32] = "xy.csv";   //Subor z ktoreho sa nacitavaju znaky do pola
char znak = '#';                //Znak ktory bude vypisovany v skicari

void setup(),menu(),vytvor(),nacitaj();
int ch;                         //Premenna do funkcie get_code();
int q;                          //Pocitacia premenna pre ukladanie do pola
int cursorPosX,cursorPosY;      //X,Y suradnice kurzora v konzole
int moznostMenu=1;              //Premenna do funkcie oznacMoznost();
char c;                         //Premenna ktoru vracia funkcia getCursorChar();
int saveData[2][2100];          //2D pole ktore uklada X,Y suradnice znakov
int rectX,rectY;                //X,Y suradnice pre tvorbu obdlznika
bool rectActive = false;        //Logicka premenna pre tvorbu obdlznika

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
            //Vypnutie aplikacie
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
            //POHYB HORE
            case ARROW_UP:
                if(cursorPosY>2){//hranica zhora
                    cursorPosY--;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            //POHYB DOLE
            case ARROW_DOWN:
                if(cursorPosY<29){//hranica zdola
                    cursorPosY++;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            //POHYB DOLAVA
            case ARROW_LEFT:
                if(cursorPosX>2){//hranica zlava
                    cursorPosX--;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            //POHYB DOPRAVA
            case ARROW_RIGHT:
                if(cursorPosX<69){//hranica sprava
                    cursorPosX++;
                    gotoxy(cursorPosX,cursorPosY);
                }
                break;
            //VPISANIE ZNAKU NA KURZORE
            case ' '://AK JE STLACENY MEDZERNIK
                printf("%c",znak);
                gotoxy(cursorPosX,cursorPosY);
                break;
            //VYTVORENIE OBDLZNIKA
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
            //ZMAZANIE ZNAKU NA KURZORE
            case 'c':
                printf(" ");
                gotoxy(cursorPosX,cursorPosY);
                break;
            //ZMAZANIE CELEJ PLOCHY
            case 'm':
                clearArea();
                gotoxy(cursorPosX,cursorPosY);
                break;
            //VYPIS KLAVES
            case 'k':
                q = 0;
                //Ulozenie X,Y suradníc do 2D pola
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
                klavesy();
                clearArea();
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
            //ULOZENIE OBRAZKA
            case 's':
                //Ulozenie X,Y suradníc do 2D pola
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
                //Ulozenie X,Y suradníc z pola do súboru
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
        printf(" X= %d   \n Y= %d   ",cursorPosX-2,cursorPosY-2);
        gotoxy(cursorPosX,cursorPosY);
    }
    gotoxy(0,32);
    //Vypnutie aplikacie
    system("taskkill/IM cb_console_runner.exe");
}

void nacitaj(){
    gotoxy(1,33);
    printf("Prebieha nacitavanie...");
    //Nacitanie X,Y suradnic do 2D pola
    getFileData(dataFile);
    //Nacitanie znakov z 2D pola
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
    //Zmena nazvu aplikacie na "Skicar"
    SetConsoleTitle("Skic\xA0r"); //\xA0 = a s dlznom
    //Vynulovanie hodnot v 2D poli
    clearSaveData();
    //Vykreslenie plochy, resp. hranic skicaru
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
