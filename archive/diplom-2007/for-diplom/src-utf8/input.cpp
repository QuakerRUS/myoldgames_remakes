#include <iostream.h>
#include <conio.h>
#include <graph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>

int main(void)
{
	FILE *fp;

	int a;
	char a1, a2;

	fp = fopen("diplom.dat", "ab");

	do
	{
		cin >> a;
		a1 = a % 256;
//		a2 = (a - a1) / 256;

		if (a != -1)
		{
			fputc(a1, fp);
//			fputc(a2, fp);
		}
	}
	while (a != -1);

	fclose(fp);

	return(1);
}
