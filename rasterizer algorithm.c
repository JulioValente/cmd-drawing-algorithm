#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WHITE 0xFF

typedef struct Square{
    int x;
    int y;
    int width;
    int height;
    float angle;
} square;

const int width = 110;
const int height = 110;

CHAR_INFO *screenBuffer;
int screenBufferCollumns = 0;
int screenBufferRows = 0;

HANDLE console;

void clearBuffer(WORD color){
    for(int j = 0; j < screenBufferRows; j++){
        for(int i = 0; i < screenBufferCollumns; i++){
            (screenBuffer + i*screenBufferRows + j)->Attributes = color;
            (screenBuffer + i*screenBufferRows + j)->Char.AsciiChar = ' ';
        }
    }
}

int setBufferSize(int row, int col){
    screenBuffer = (CHAR_INFO *)malloc(sizeof(CHAR_INFO)*row*col);

    if(screenBuffer != NULL){
        screenBufferCollumns = col;
        screenBufferRows = row;
        clearBuffer(0);

        return 1;
    }else{
        return 0;
    }
}

void setConsoleBuffer(){
    COORD bufferSize;
    COORD drawPosition;
    SMALL_RECT drawRegion;

    bufferSize.X = screenBufferRows;
    bufferSize.Y = screenBufferCollumns;

    drawPosition.X = 0;
    drawPosition.Y = 0;

    drawRegion.Top = 0;
    drawRegion.Left = 0;
    drawRegion.Bottom = screenBufferCollumns-1;
    drawRegion.Right = screenBufferRows-1;

    WriteConsoleOutput(console, screenBuffer, bufferSize, drawPosition, &drawRegion);
}

void drawPixel(int x, int y, WORD color){
    (screenBuffer + x + y*screenBufferRows)->Attributes = color;
}

void drawChar(int x, int y, char c){
    (screenBuffer + x + y*screenBufferRows)->Char.AsciiChar = c;
}

void setFontSize(int size){

    COORD fontSize = {size, size};
    CONSOLE_FONT_INFOEX fontInformation = {sizeof(fontInformation), 0, fontSize, 0, FW_NORMAL, 0};
    wcscpy(fontInformation.FaceName, L"Terminal");

    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), 1, &fontInformation); //seta a fonte do console
}

void hidecursor(){
   HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
   CONSOLE_CURSOR_INFO info;
   info.dwSize = 100;
   info.bVisible = 0;
   SetConsoleCursorInfo(consoleHandle, &info);
}

inline static int random(int a, int b){
    return round(((float)rand()*(float)(b-a))/(float)RAND_MAX) + a;
}

void rasterize(COORD *left, COORD *right, int vSize){
    int x;
    for(int i=1;i<vSize;i++){
        x = (left+i)->X;
        while(x < (right+i)->X - 1){
            x++;
            drawChar(x, (left+i)->Y, random(0, 0xFF));
        }
    }
}

void drawLine(int x1, int y1, int x2, int y2, WORD color, int left, COORD *v, int downMostY){
    int x;
    int y;

    int dx = abs(x2-x1);
    int dy = abs(y2-y1);
    dx = dx << 1;
    dy = dy << 1;

    int inc;
    if(dy<dx){
        if(y2<y1)
            inc = -1;
        else if(y2>y1)
            inc = 1;
        else
            inc=0;

        if(x1 <= x2){
            x = x1;
            y = y1;
            if(y2<y1)
                inc = -1;
            else if(y2>y1)
                inc = 1;
            else
                inc=0;
        }else{
            x = x2;
            y = y2;
            if(y1<y2)
                inc = -1;
            else if(y1>y2)
                inc = 1;
            else
                inc=0;
        }

        int p = dy - (dx>>1);
        int yIncremented;
        int endx;

        if(x1<x2)
            endx = x2;
        else
            endx = x1;

        if(left){
            while(x<=endx){
                //drawPixel(x, y, color);

                (v+y-downMostY)->X = x;

                if(p<0){
                    p = p + dy;
                }else{
                    p = p + dy - dx;
                    (v+y-downMostY)->Y = y;
                    y+=inc;
                }

                x++;
            }
        }
        else{
            while(x<=endx){
                //drawPixel(x, y, color);

                if(yIncremented){
                    (v+y-downMostY)->X = x;
                    (v+y-downMostY)->Y = y;
                    yIncremented = 0;
                }

                if(p<0){
                    p = p + dy;
                }else{
                    p = p + dy - dx;
                    yIncremented = 1;
                    y+=inc;
                }

                x++;
            }
        }
    }else{
        if(y1 <= y2){
            y = y1;
            x = x1;
            if(x2<x1)
                inc = -1;
            else if(x2>x1)
                inc = 1;
            else
                inc=0;
        }else{
            y = y2;
            x = x2;
            if(x1<x2)
                inc = -1;
            else if(x1>x2)
                inc = 1;
            else
                inc=0;
        }

        int p = dx - (dy>>1);
        int endy;

        if(y1<y2)
            endy = y2;
        else
            endy = y1;

        while(y<=endy){
            //drawPixel(x, y, color);

            (v+y-downMostY)->X = x;
            (v+y-downMostY)->Y = y;

            if(p<0){
                p = p + dx;
            }else{
                p = p + dx - dy;
                x+=inc;
            }

            y++;
        }
    }
}

void drawRotationalSquare(square square){
    float cornerDistance = hypotf((float)square.width, (float)square.height)/(float)2;
    float cornerAngle =  acosf( (float)square.width/(2*cornerDistance) );
    COORD corners[4];

    float x = (float)(square.x + (square.width>>1));
    float y = (float)(square.y + (square.height>>1));

    float c1 = cosf(cornerAngle - square.angle)*cornerDistance;
    float c2 = cosf(cornerAngle + square.angle)*cornerDistance;
    corners[0].X = (int)(x + c1 + 0.5);
    corners[1].X = (int)(x - c2 + 0.5);
    corners[2].X = (int)(x - c1 + 0.5);
    corners[3].X = (int)(x + c2 + 0.5);

    c1 = sinf(cornerAngle - square.angle)*cornerDistance;
    c2 = sinf(cornerAngle + square.angle)*cornerDistance;
    corners[0].Y = (int)(y - c1 + 0.5);
    corners[1].Y = (int)(y - c2 + 0.5);
    corners[2].Y = (int)(y + c1 + 0.5);
    corners[3].Y = (int)(y + c2 + 0.5);

    int leftIndex[4];
    int greaterIndex;
    int smallerIndex;
    float quartet = (float)M_PI/(float)2;
    if(square.angle < quartet){
        int array[4] = {0, 1, 1, 0};
        memcpy(leftIndex, array, sizeof(leftIndex));
        greaterIndex = 3;
        smallerIndex = 1;
    }else if(square.angle < quartet*(float)2){
        int array[4] = {0, 0, 1, 1};
        memcpy(leftIndex, array, sizeof(leftIndex));
        greaterIndex = 0;
        smallerIndex = 2;
    }else if(square.angle < quartet*(float)3){
        int array[4] = {1, 0, 0, 1};
        memcpy(leftIndex, array, sizeof(leftIndex));
        greaterIndex = 1;
        smallerIndex = 3;
    }else{
        int array[4] = {1, 1, 0, 0};
        memcpy(leftIndex, array, sizeof(leftIndex));
        greaterIndex = 2;
        smallerIndex = 0;
    }

    float octet = (float)M_PI/(float)4;
    float side = (square.angle < (float)M_PI)?(square.angle):(square.angle - (float)M_PI);
    int drawOrder[4];
    if(side < octet){
        int array[4] = {1, 3, 0, 2};
        memcpy(drawOrder, array, sizeof(drawOrder));
    }else if (side < octet*(float)2){
        int array[4] = {0, 2, 1, 3};
        memcpy(drawOrder, array, sizeof(drawOrder));
    }else if (side < octet*(float)3){
        int array[4] = {0, 2, 1, 3};
        memcpy(drawOrder, array, sizeof(drawOrder));
    }else{
        int array[4] = {1, 3, 0, 2};
        memcpy(drawOrder, array, sizeof(drawOrder));
    }


    COORD left[corners[greaterIndex].Y - corners[smallerIndex].Y]; //size = distance between the upmost and downmost points
    COORD right[corners[greaterIndex].Y - corners[smallerIndex].Y]; //size = distance between the upmost and downmost points

    for(int i=0; i < 4; i++){
        int k = drawOrder[i];
        int j = k;
        if(j!=3)
            j++;
        else
            j=0;

        if(leftIndex[k])
            drawLine(corners[k].X, corners[k].Y, corners[j].X, corners[j].Y, WHITE, 1, &left, corners[smallerIndex].Y);
        else
            drawLine(corners[k].X, corners[k].Y, corners[j].X, corners[j].Y, WHITE, 0, &right, corners[smallerIndex].Y);
    }
    rasterize(&left, &right, corners[greaterIndex].Y - corners[smallerIndex].Y);
}

setConsoleParameters(){

    setFontSize(8);
    hidecursor();

    SMALL_RECT consoleWindowSize;
    COORD consoleBufferSize;

    consoleWindowSize.Top = 0;
    consoleWindowSize.Left = 0;
    consoleWindowSize.Right = width;
    consoleWindowSize.Bottom = height;

    consoleBufferSize.X = consoleWindowSize.Right + 1;
    consoleBufferSize.Y = consoleWindowSize.Bottom + 1;

    if(!SetConsoleWindowInfo(console, 1, &consoleWindowSize)){
        printf("\a");
    }
    if(!SetConsoleScreenBufferSize(console, consoleBufferSize)){
        printf("\a");
    }

    SetConsoleTitle("rasterizer algorithm");
}

int main(){
    srand((unsigned)time(NULL));

    console = GetStdHandle(STD_OUTPUT_HANDLE);

    setConsoleParameters();

    if(!setBufferSize(width+1, height+1)){
        printf("\a");

        while(!GetAsyncKeyState(VK_ESCAPE));
        return 1;
    }

    square square = {25, 25, 60, 60, (float) 0 * (float)M_PI / (float)180};
    drawRotationalSquare(square);
    setConsoleBuffer();

    while(!GetAsyncKeyState(VK_ESCAPE)){
        square.angle+=(float)M_PI/(float)180;
        if(square.angle >= (float)M_PI*(float)2)
            square.angle = 0;

        clearBuffer(0x02);
        drawRotationalSquare(square);
        setConsoleBuffer();

        Sleep(1);
    }

    return 0;
}

