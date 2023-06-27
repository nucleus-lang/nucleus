#ifndef SAY_C
#define SAY_C

#include <stdio.h>

#ifdef _WIN32
	#include <Windows.h>
#endif

int say(char* string, int length)
{
	int output = 0;

	#ifdef _WIN32
		HANDLE out = GetStdHandle(-11);

		DWORD bytes_written;
		output = WriteFile(out, string, length, &bytes_written, NULL) == FALSE;
	#endif

	return output;
}

int say_i8_number(char n)
{
	char res[4];
	itoa(n, res, 10);

	return say(res, 4);
}

#endif