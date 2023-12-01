#ifndef STRING_PARSER_H_
#define STRING_PARSER_H_


#define _GUN_SOURCE


typedef struct
{
    char** command_list;
    int num_token;
}command_line;

int count_token (char* buf, const char* delim); // counts tokens in buf, fills it as num_token

command_line str_filler (char* buf, const char* delim); // parses tokens in buf, appends them to command_list

void free_command_line(command_line* command); // free func (cracked)


#endif