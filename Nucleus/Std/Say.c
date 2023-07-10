#ifndef SAY_C
#define SAY_C

#include <stdio.h>

#ifdef _WIN32
	#include <Windows.h>
#endif

int say(char* string, int length)
{
	int output = 0;

	int get_length = 0;
	for(int i = 0; i < length; i++)
	{
		get_length += 1;

		if(string[i] == 0)
			break;
	}

	#ifdef _WIN32
		HANDLE out = GetStdHandle(-11);

		DWORD bytes_written;
		output = WriteFile(out, string, get_length, &bytes_written, NULL) == FALSE;
	#endif

	return output;
}

char input_error_msg[53] = "Sorry, console input is bigger than the Array Length.";

int input(char* string, int length)
{
	int res = fgets(string, length, stdin) != NULL;

	if(res)
	{
		for(int i = 0; i < length; i++)
		{
			if(string[i] == '\n')
				string[i] = '\0';
		}

		return 0;
	}

	return 1;
}

char i8_to_string_res[4];
char* i8_to_string(char n)
{
	itoa(n, i8_to_string_res, 10);

	return i8_to_string_res;
}

int say_i8_number(char n)
{
	return say(i8_to_string(n), 4);
}

#endif