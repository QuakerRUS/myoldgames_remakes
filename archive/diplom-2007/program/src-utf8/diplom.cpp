#include <iostream.h>
#include <conio.h>
#include <graph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <malloc.h>

#define MAXX 640
#define MAXY 400
#define FONTQ 95
#define FONTRAWS 14
#define FONTCOLUMNS 11
#define MAXLENSTRING 255
#define STRINGLEN 41
#define FIRSTLETTERX 14
#define FIRSTLETTERY 39
#define ENDLETTERX 482
#define ENDLETTERY 154
#define KEYBIMAGEFX 30
#define KEYBIMAGEFY 170
#define KEYBIMAGELX 609
#define KEYBIMAGELY 364
#define HINTBORDER 3
#define HINTCOLOR 241
#define SPEEDFX 294
#define SPEEDFY 141

void initProgram(void);
void loadData(void);
void exitProgram(void);
void anyKey(void);
void palette(int, int, int);
void typeLetter(char);
void typeNumPos(int, int, int);
void typeText();
void newString();
char recodeLetter(char, int);
void recodeString();
void formatText();
void hintKey(int);
void printSpeed();

FILE *fileText;
struct dostime_t time;

char fonts[FONTQ][FONTRAWS][FONTCOLUMNS], string1[MAXLENSTRING],
     string2[MAXLENSTRING], symbolsEngToRus[FONTQ],
     symbolsCodeTable[18] = {45, 61, 33, 34, 252, 59, 37, 58, 63,
                            42, 40, 41, 95, 43, 92, 47, 46, 44},
     *letterBuffer, *keybImageBuffer;
int symbol1, symbol2, string1Len, string2Len, keyCoords[FONTQ + 2][4],
    letterX, letterY, cursorPosition, speed = 0, start = 0, startTime, curTime;
long colors[256];

int main(void)
{
	initProgram();

	do
	{
		anyKey();

		if (recodeLetter(symbol1, 1) == string1[cursorPosition] ||
		   recodeLetter(symbol1, 0) == 66 &&
		   string1[cursorPosition] == FONTQ + 2)
		{
			_putimage(KEYBIMAGEFX, KEYBIMAGEFY, keybImageBuffer, _GPSET);
			_putimage(letterX, letterY, letterBuffer, _GPSET);

			if (recodeLetter(symbol1, 0) == 66 &&
			   string1[cursorPosition] == FONTQ + 2)
			{
				letterX = FIRSTLETTERX;
				letterY += FONTRAWS;
				string1[cursorPosition] = 66;
			}
			else
				letterX += FONTCOLUMNS;

			cursorPosition++;

			_getimage(letterX, letterY, letterX + FONTCOLUMNS - 1,
	                 letterY + FONTRAWS - 1, letterBuffer);
			_setcolor(15);
			_rectangle(_GBORDER, letterX, letterY, letterX + FONTCOLUMNS - 1,
			          letterY + FONTRAWS - 1);

			if (start == 0)
			{
				_dos_gettime(&time);
				startTime = time.second + time.minute * 60 + time.hour * 3600;
			}

			start++;
			_dos_gettime(&time);
			curTime = time.second + time.minute * 60 + time.hour * 3600;

			if (curTime - startTime >= 2)
			{
				speed = start * 60 / (curTime - startTime);
				printSpeed();
			}

			if (string1[cursorPosition] != FONTQ + 2)
				hintKey(string1[cursorPosition]);
			else
				hintKey(66);

			if (string1[cursorPosition - 1] == 93 ||
			   string1[cursorPosition - 1] == 79 ||
			   string1[cursorPosition - 1] == 85)
				newString();
		}
	}
	while (symbol1 != 27);

	exitProgram();

	return(1);
}

// Подпрограмма иницализации программы

void initProgram(void)
{
	FILE *fp;

	int fpPicPos;

	_setvideomode(_URES256COLOR);
	_dos_gettime(&time);
	srand( (long) time.hsecond + (long) time.second * 100 +
	     (long) time.minute * 6000 + (long) time.hour * 360000);
	letterBuffer = (char*) malloc(_imagesize(0, 0, FONTCOLUMNS, FONTRAWS) );
	keybImageBuffer = (char*) malloc(_imagesize(KEYBIMAGEFX, KEYBIMAGEFY,
	                  KEYBIMAGELX, KEYBIMAGELY) );
	fp = fopen("Data/Images/bg.bmp", "rb");
	fseek(fp, 10, SEEK_SET);
	fpPicPos = fgetc(fp);
	fpPicPos += fgetc(fp) * 256;
	fseek(fp, 54, SEEK_SET);

	for (int i = 0; i < 256; i++)
	{
		colors[i] = fgetc(fp) / 4 * 65536;
		colors[i] += fgetc(fp) / 4 * 256;
		colors[i] += fgetc(fp) / 4;
		fgetc(fp);
	}

	palette (100, 100, 100);

	fseek(fp, fpPicPos, SEEK_SET);

	for (int j = MAXY - 1; j >= 0; j--)
		for (int i = 0; i < MAXX; i++)
		{
			_setcolor(fgetc(fp) );
			_setpixel(i, j);
		}

	fclose(fp);

	fp = fopen("Data/Images/fonts.bmp", "rb");
	fseek(fp, 10, SEEK_SET);
	fpPicPos = fgetc(fp);
	fpPicPos += fgetc(fp) * 256;
	fseek(fp, fpPicPos, SEEK_SET);

	for (int f = FONTQ - 1; f >= 0; f--)
		for (int j = FONTRAWS - 1; j >= 0; j--)
		{
			for (int i = 0; i < FONTCOLUMNS; i++)
				fonts[f][j][i] = fgetc(fp);

			if (FONTCOLUMNS % 2 == 1)
				fgetc(fp);
		}

	_getimage(KEYBIMAGEFX, KEYBIMAGEFY, KEYBIMAGELX, KEYBIMAGELY,
	         keybImageBuffer);
	fileText = fopen("Data/Texts/text_rus.txt", "r");
	for (int i = rand() % 3000; i >= 0; i--)
		fgets(string2, MAXLENSTRING, fileText);

	formatText();
   	loadData();
	newString();
}

// Подпрограмма ввода данных из файла diplom.dat

void loadData(void)
{
	FILE *dataFile;

	dataFile = fopen("Data/diplom.dat", "rb");

	for (int i = 0; i < FONTQ + 2; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			keyCoords[i][j] = fgetc(dataFile);
			keyCoords[i][j] += fgetc(dataFile) * 256;
		}
	}

	for (int i = 0; i < FONTQ; i++)
		symbolsEngToRus[i] = fgetc(dataFile);

	fclose(dataFile);
}

// Подпрограмма зевершения работы программы

void exitProgram(void)
{
	fclose(fileText);
	_setvideomode(_DEFAULTMODE);
}

// Подпрограмма ожидания нажатия произвольной клавиши

void anyKey(void)
{
	symbol1 = getch();

	if (symbol1 == 0)
		symbol2 = getch();
}

// Подпрограмма работы с палитрой

void palette(int red, int green, int blue)
{
	long r, g, b;

	for (int i = 0; i < 256; i++)
	{
		r=colors[i] % 256;
		g=(colors[i] - r) % 65536 / 256;
		b=(colors[i] - g - r) / 65536;
		_remappalette (i, r * (long) red / 100 + g * (long) green / 100 * 256 +
		              b * (long) blue / 100 * 65536);
	}
}

// Подпрограмма отображения буквы на экране

void typeLetter(char letter)
{
	for (int j = 0; j < FONTRAWS; j++)
		for (int i = 0; i < FONTCOLUMNS; i++)
		{
			_setcolor(fonts[letter][j][i]);
			_setpixel(letterX + i, letterY + j);
		}

	letterX += FONTCOLUMNS;
}

// Подпрограмма отображения цифры на экране

void typeNumPos(int num, int x, int y)
{
	int letter;

	if (num >= 1 && num <= 9)
		letter = num + 66;

	if (num == 0)
		letter = 76;

	for (int i = 0; i < FONTRAWS; i++)
		for (int j = 0; j < FONTCOLUMNS; j++)
		{
			_setcolor(fonts[letter][i][j]);
			_setpixel(x + j, y + i);
		}
}

// Подпрограмма отображения текста на экране

void typeText(void)
{
	for (int i = 0; i < string1Len; i++)
	{
		if (string1[i] != FONTQ + 2)
			typeLetter(string1[i]);

		if (string1[i] == FONTQ + 2 || string1[i] == 93 || string1[i] == 79 ||
		   string1[i] == 85)
		{
			letterX = FIRSTLETTERX;
			letterY += FONTRAWS;
		}
	}

	for (int i = 0; i < string2Len; i++)
	{
		if (string2[i] != FONTQ + 2)
			typeLetter(string2[i]);

		if (string2[i] == FONTQ + 2)
		{
			letterX = FIRSTLETTERX;
			letterY += FONTRAWS;
		}
	}
}

// Подпрограмма получения следующей строки из файла

void newString(void)
{
	_setcolor (_getpixel(ENDLETTERX, ENDLETTERY) );
	_rectangle (_GFILLINTERIOR, FIRSTLETTERX, FIRSTLETTERY, ENDLETTERX,
	           ENDLETTERY - FONTRAWS);
	strcpy(string1, string2);
	string1Len = string2Len;
	fgets(string2, MAXLENSTRING, fileText);
	letterX = FIRSTLETTERX;
	letterY = FIRSTLETTERY;
	formatText();
	typeText();
	letterX = FIRSTLETTERX;
	letterY = FIRSTLETTERY;
	cursorPosition = 0;
	_getimage(letterX, letterY, letterX + FONTCOLUMNS - 1,
	         letterY + FONTRAWS - 1, letterBuffer);
	_setcolor(15);
	_rectangle(_GBORDER, letterX, letterY, letterX + FONTCOLUMNS - 1,
	          letterY + FONTRAWS - 1);
	_putimage(KEYBIMAGEFX, KEYBIMAGEFY, keybImageBuffer, _GPSET);
	hintKey(string1[cursorPosition]);
}

// Подпрограмма перекодировки буквы

char recodeLetter(char letter, int engToRus)
{
	char recodedLetter;

	recodedLetter = 255;

	if (engToRus == 1)
	{
		int i = 0;

		do
		{
			if (letter == symbolsEngToRus[i])
				recodedLetter = i;

			i++;
		}
		while (recodedLetter == 255 && i < FONTQ);
	}
	else if (letter >= 128 && letter <= 132)
		recodedLetter = letter - 128;
	else if (letter == 240)
		recodedLetter = 6;
	else if (letter >= 134 && letter <= 165)
		recodedLetter = letter - 127;
	else if (letter == 241)
		recodedLetter = 39;
	else if (letter >= 166 && letter <= 175)
		recodedLetter = letter - 126;
	else if (letter >= 224 && letter <= 239)
		recodedLetter = letter - 174;
	else if (letter == 32)
		recodedLetter = 66;
	else if (letter >= 48 && letter <= 57)
		recodedLetter = letter + 19;
	else
	{
		int i = 0;

		do
		{
			if (letter == symbolsCodeTable[i])
				recodedLetter = i + 77;

			i++;
		}
		while (recodedLetter == 255 && i < 18);
	}

	return (recodedLetter);
}

// Подпрограмма перекодировки строки

void recodeString(void)
{
	string2Len = -1;

	do
	{
		string2Len++;
		string2[string2Len] = recodeLetter(string2[string2Len], 0);
	}
	while (string2[string2Len] != 93 &&
	      string2[string2Len] != 79 &&
	      string2[string2Len] != 85);

	string2Len++;
}

// Подпрограмма форматирования текста

void formatText(void)
{
	int rowStart = 0, wordStart = 0, curLetter;

	recodeString();

	do
	{
		curLetter = rowStart;

		while (curLetter - rowStart < STRINGLEN &&
		      string2[curLetter] != 93 && string2[curLetter] != 79 &&
		      string2[curLetter] != 85)
		{
			if (string2[curLetter] == 66)
				wordStart = curLetter + 1;

			curLetter++;
		}

		if (curLetter - rowStart == STRINGLEN)
		{
			rowStart = wordStart;
			string2[rowStart - 1] = FONTQ + 2;
		}
	}
	while (string2[curLetter] != 93 && string2[curLetter] != 79 &&
	      string2[curLetter] != 85);
}

// Подпрограмма отображения подсказкок на клавиатуре

void hintKey(int letter)
{
	_setcolor(HINTCOLOR);

	for (int i = 0; i < HINTBORDER; i++)
		_rectangle(_GBORDER, keyCoords[letter][0] + i,
		          keyCoords[letter][1] + i, keyCoords[letter][2] - i,
		          keyCoords[letter][3] - i);
}

// Подпрограмма отображения скорости печати

void printSpeed(void)
{
	int speedX = SPEEDFX;

	_setcolor (_getpixel(ENDLETTERX, ENDLETTERY) );
	_rectangle (_GFILLINTERIOR, SPEEDFX, SPEEDFY, ENDLETTERX, ENDLETTERY);

	speed %= 100000;

	if (speed >= 10000)
	{
		typeNumPos(speed % 100000 / 10000, speedX, SPEEDFY);
		speedX += FONTCOLUMNS;
	}

	if (speed >= 1000)
	{
		typeNumPos(speed % 10000 / 1000, speedX, SPEEDFY);
		speedX += FONTCOLUMNS;
	}

	if (speed >= 100)
	{
		typeNumPos(speed % 1000 / 100, speedX, SPEEDFY);
		speedX += FONTCOLUMNS;
	}

	if (speed >= 10)
	{
		typeNumPos(speed % 100 / 10, speedX, SPEEDFY);
		speedX += FONTCOLUMNS;
	}

	typeNumPos(speed % 10, speedX, SPEEDFY);
}
