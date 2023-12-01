#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_parser.h"

#define _GUN_SOURCE

int count_token (char* buf, const char* delim)
{
	/*
	*	#1.	Check for NULL string
	*	#2.	iterate through string counting tokens
	*		Cases to watchout for
	*			a.	string start with delimeter
	*			b. 	string end with delimeter
	*			c.	account NULL for the last token
	*	#3. return the number of token (note not number of delimeter)
	*/

	char* str1 = malloc(strlen(buf)+ 1);
	char* temp = strcpy(str1, buf); // we will use this to 
	str1 = strtok(str1, "\n"); // no newline
	//printf("%s\n", str1);
	char* token;
	int count = 0;

	if (buf == NULL)
	{
		return 0;
	}

	while((token = strtok_r(str1, delim, &str1)))
	{
		//printf("Token: %s<-\n", token);
		count++;
	}

	free(temp);
	return count;
}

command_line str_filler (char* buf, const char* delim)
{
	/*
	*	#1.	create command_line variable to be filled and returned
	*	#2.	count the number of tokens with count_token function, set num_token. 
    	*           one can use strtok_r to remove the \n at the end of the line.
	*	#3. malloc memory for token array inside command_line variable
	*	    based on the number of tokens.
	*	#4.	use function strtok_r to find out the tokens 
    	*       #5. malloc each index of the array with the length of tokens,
	*	    fill command_list array with tokens, and fill last spot with NULL.
	*	#6. return the variable.
	*/

	char *token, *str1, *saveptr1, *test;
	int t = 0;

	command_line line;
	int temp = count_token(buf, delim);
	if(temp != 0)
	{
		line.num_token = temp;
	} else 
	{
		printf("PROBLEM\n");
		exit(1);
	}
	//printf("NUM TOKEN: %d\n", line.num_token);
	//line.command_list = (char **)malloc(sizeof(char *) * (line.num_token + 1));
	char **tmp = (char **)malloc((sizeof(char *)) * line.num_token);
	if(tmp != NULL)
	{
		line.command_list = tmp;
	} else 
	{
		printf("We got a prob over here in str_filler\n");
		exit(1);
	}

	//printf("line_buf: %s<-\n", buf);	
	buf = strtok(buf, "\n"); // Get rid of newline char
	//printf("line_buf: %s<-\n", buf);	
	
	token = strtok_r(buf, delim, &saveptr1);
	//printf("Token: %s<-\n", token);
	while (token != NULL)
	{
		token = strtok(token, "\n");
		line.command_list[t] = token;
		//printf("%s\n", line.command_list[t]);
		t++;
		token = strtok_r(NULL, delim, &saveptr1);
	}
	
	return line;
}


void free_command_line(command_line* command)
{
	free(command->command_list);
}