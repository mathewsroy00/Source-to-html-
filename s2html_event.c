#include <stdio.h>
#include <string.h>
#include "s2html_event.h"

#define SIZE_OF_SYMBOLS    (sizeof(symbols))
#define SIZE_OF_OPERATORS  (sizeof(operators))
#define WORD_BUFF_SIZE      100

// Internal states and event of parser

typedef enum
{
    PSTATE_IDLE,                      // Default state
    PSTATE_PREPROCESSOR_DIRECTIVE,  
    PSTATE_SUB_PREPROCESSOR_MAIN, 
    PSTATE_SUB_PREPROCESSOR_RESERVE_KEYWORD,
    PSTATE_SUB_PREPROCESSOR_ASCII_CHAR,
    PSTATE_HEADER_FILE,
    PSTATE_RESERVE_KEYWORD,
    PSTATE_NUMERIC_CONSTANT,
    PSTATE_STRING,
    PSTATE_SINGLE_LINE_COMMENT,
    PSTATE_MULTI_LINE_COMMENT,
    PSTATE_ASCII_CHAR
}pstate_e;

// Global variables

static pstate_e state = PSTATE_IDLE;  //current state of parser

static pstate_e state_sub = PSTATE_SUB_PREPROCESSOR_MAIN; // Substate is used only in preprocessor state

static pevent_t pevent_data;        // event variable to store event and related properties
static int event_data_idx = 0;

static char word[WORD_BUFF_SIZE];    // temperory array
static int word_idx = 0;

static char* res_kwords_data[] = {"const", "volatile", "extern", "auto", "register",
   						   "static", "signed", "unsigned", "short", "long", 
						   "double", "char", "int", "float", "struct", 
						   "union", "enum", "void", "typedef", ""
						  };

static char* res_kwords_non_data[] = {"goto", "return", "continue", "break", 
							   "if", "else", "for", "while", "do", 
							   "switch", "case", "default","sizeof", ""
							  };

static char operators[] = {'/', '+', '*', '-', '%', '=', '<', '>', '~', '&', ',', '!', '^', '|'};
static char symbols[] = {'(', ')', '{', '[', ':'};

/********** state handlers **********/
pevent_t * pstate_idle_handler(FILE *fd, int ch);
pevent_t * pstate_single_line_comment_handler(FILE *fd, int ch);
pevent_t * pstate_multi_line_comment_handler(FILE *fd, int ch);
pevent_t * pstate_numeric_constant_handler(FILE *fd, int ch);
pevent_t * pstate_string_handler(FILE *fd, int ch);
pevent_t * pstate_header_file_handler(FILE *fd, int ch);
pevent_t * pstate_ascii_char_handler(FILE *fd, int ch);
pevent_t * pstate_reserve_keyword_handler(FILE *fd, int ch);
pevent_t * pstate_preprocessor_directive_handler(FILE *fd, int ch);
pevent_t * pstate_sub_preprocessor_main_handler(FILE *fd, int ch);

// Utility functions

// Function to check if given word is reserved keyword
static int is_reserved_keyword(char* word)
{
    // Searching for data type reserved keyword
    for(int i = 0; res_kwords_data[i][0] != '\0'; i++)
    {
        if(strcmp(res_kwords_data[i], word) == 0)
            return RES_KEYWORD_DATA;
    }

    // Search for non data type reserved keyword
    for(int i = 0; res_kwords_non_data[i][0] != '\0'; i++)
    {
        if(strcmp(res_kwords_non_data[i], word) == 0)
            return RES_KEYWORD_NON_DATA;
    }

    return 0; // If the words did not match, returning false
}

// Function to check symbols
static int is_symbol(char ch)
{
    for(int i = 0; i < SIZE_OF_SYMBOLS; i++)
    {
        if(symbols[i] == ch)
            return 1;
    }

    return 0;
}

// Checks if character is an operator
static int is_operator(char ch)
{
    for(int i = 0; i < SIZE_OF_OPERATORS; i++)
    {
        if(operators[i] == ch)
            return 1;
    }

    return 0;
}

// To set parser event calls every time a token is complete
// Finalizes current token and prepares for next
static void set_parser_event(pstate_e s, pevent_e e)
{
    pevent_data.data[event_data_idx] = '\0';     // terminates the collected characters in buffer
    pevent_data.length = event_data_idx;        // stores how many characters were collected into the length field
    event_data_idx = 0;                        // resetting the bufffer index back to 0 ready for next token
    state = s;                                 
    pevent_data.type = e;
}

// Event functions

/* This function parses the source file and generate 
 * event based on parsed characters and string
 */
pevent_t *get_parser_event(FILE* fd)
{
    int ch, pre_ch;
    pevent_t *evptr = NULL;

    // Reading char by char until end of file
    while((ch =fgetc(fd)) != EOF)
    {
        switch(state)
        {
            case PSTATE_IDLE:
                if((evptr = pstate_idle_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            case PSTATE_SINGLE_LINE_COMMENT:
                if((evptr = pstate_single_line_comment_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            case PSTATE_MULTI_LINE_COMMENT:
                if((evptr = pstate_multi_line_comment_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            case PSTATE_PREPROCESSOR_DIRECTIVE:
                if((evptr = pstate_preprocessor_directive_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            case PSTATE_RESERVE_KEYWORD:
                if((evptr = pstate_reserve_keyword_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            case PSTATE_HEADER_FILE:
                if((evptr = pstate_header_file_handler(fd, ch)) != NULL)
                    return evptr;
                break;
            
            case PSTATE_NUMERIC_CONSTANT:
                if((evptr = pstate_numeric_constant_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            case PSTATE_STRING:
                if((evptr = pstate_string_handler(fd, ch)) != NULL)
                    return evptr;
                break;
            
            case PSTATE_ASCII_CHAR:
                if((evptr = pstate_ascii_char_handler(fd, ch)) != NULL)
                    return evptr;
                break;

            default:
                printf("Unknown state\n");
                state = PSTATE_IDLE;
                break;
        }
    }

    // After reaching eof move back to idle state and set EOF event
    set_parser_event(PSTATE_IDLE, PEVENT_EOF);

    return &pevent_data;  // return final event
}

// Idle state handler
// Handles default state and detects start of tokens
pevent_t *pstate_idle_handler(FILE* fd, int ch)
{
    int pre_ch;
    switch(ch)
    {
        case '\'':    // beginning of ASCII char
            if(event_data_idx)       // processing regular expression first
            {
                ungetc(ch, fd);
                set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                return &pevent_data;
            }
            else
            {
                state = PSTATE_ASCII_CHAR;
                pevent_data.data[event_data_idx++] = ch;
            }
            break;
        
        case '/':                   // comment or operator
            pre_ch = ch;
            if((ch = fgetc(fd)) == '*')      // Multi-line comment
            {
                if(event_data_idx)         // if we have regular expression in buffer first processs that
                {
                    fseek(fd, -2L, SEEK_CUR);  // moving back 2 bytes 
                    set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                    return &pevent_data;
                }
                else // multi line comment begin
                {
 #ifdef DEBUG	
					printf("Multi line comment Begin : /*\n");
#endif               
                    state = PSTATE_MULTI_LINE_COMMENT;
                    pevent_data.data[event_data_idx++] = pre_ch;
                    pevent_data.data[event_data_idx++] = ch;
                }
            }
            else if(ch == '/')     // single line comment
            {
                if(event_data_idx)
                {
                    fseek(fd, -2L, SEEK_CUR);  // moving back 2 bytes
                    set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                    return &pevent_data;
                }
                else  // single line comment begin
                {
#ifdef DEBUG	
					printf("Single line comment Begin : //\n");
#endif                   
                    state = PSTATE_SINGLE_LINE_COMMENT;
                    pevent_data.data[event_data_idx++] = pre_ch;
                    pevent_data.data[event_data_idx++] = ch;
                }
            }
            else // it is a regular exp
            {
                pevent_data.data[event_data_idx++] = pre_ch;
                pevent_data.data[event_data_idx++] = ch;
            }
            break;

        case '#':                   // beginning of preprocessor directives
            if(event_data_idx)
            {
                ungetc(ch, fd);       // Put back '#'
                set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                return &pevent_data;
            }
            else
            {
                pevent_data.data[event_data_idx++] = ch;
                state = PSTATE_PREPROCESSOR_DIRECTIVE;
                state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;
            }

            break;

        case '\"':              // start of string
            if(event_data_idx)
            {
                ungetc(ch, fd);   /* put back '"' */
                set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                return &pevent_data;
            }
            else
            {
                state = PSTATE_STRING;
                pevent_data.data[event_data_idx++] = ch; 
            }

            break;

        case '0' ... '9':         // Numeric constant
            if(event_data_idx)
            {
                ungetc(ch, fd);
                set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                return &pevent_data;
            }
            else
            {
                state = PSTATE_NUMERIC_CONSTANT;
                pevent_data.data[event_data_idx++] = ch;
            }

            break;

        case 'a' ... 'z':           // Keyword or identifier
            if(event_data_idx)
            {
                ungetc(ch, fd);
                set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
                return &pevent_data;
            }
            else
            {
                state = PSTATE_RESERVE_KEYWORD;
                word_idx = 0;
                word[word_idx++] = ch;
                pevent_data.data[event_data_idx++] = ch;
            }

            break;

        default:
                pevent_data.data[event_data_idx++] = ch;
                break;
    }
    return NULL;
}

// Handles preprocessor directive parsing
pevent_t *pstate_preprocessor_directive_handler(FILE* fp, int ch)
{
    switch(state_sub)
    {
        case PSTATE_SUB_PREPROCESSOR_MAIN:
            return pstate_sub_preprocessor_main_handler(fp, ch);
        
        case PSTATE_SUB_PREPROCESSOR_RESERVE_KEYWORD:
            return pstate_reserve_keyword_handler(fp, ch);

        case  PSTATE_SUB_PREPROCESSOR_ASCII_CHAR:
            return pstate_ascii_char_handler(fp, ch);

        case PSTATE_HEADER_FILE:
            return pstate_header_file_handler(fp, ch);

        default:
            printf("Unknown state\n");
            state = PSTATE_IDLE;
    }
    return NULL;
}

// Handles main part of preprocessor directive
pevent_t *pstate_sub_preprocessor_main_handler(FILE *fd, int ch)
{
    switch(ch)
    {
        case ' ':  // space means preprocessor directive word is dont
            pevent_data.data[event_data_idx++] = ch;
            set_parser_event(PSTATE_PREPROCESSOR_DIRECTIVE, PEVENT_PREPROCESSOR_DIRECTIVE);
            return &pevent_data;

        case '\n':
            ungetc(ch, fd);
            if(event_data_idx > 0)
            {
                set_parser_event(PSTATE_IDLE, PEVENT_PREPROCESSOR_DIRECTIVE);
                return &pevent_data;
            }
            state = PSTATE_IDLE;
            break;

        case '<':    // system header file begins
            state_sub = PSTATE_HEADER_FILE;
            pevent_data.property = STD_HEADER_FILE;
            break;

        case '\"':   // user header file begins
            pevent_data.data[event_data_idx++] = ch;
            state_sub = PSTATE_HEADER_FILE;
            pevent_data.property = USER_HEADER_FILE;
            break;

        case '\'':   // ascii char inside preprocessor
            state_sub = PSTATE_SUB_PREPROCESSOR_ASCII_CHAR;
            pevent_data.data[event_data_idx++] = ch;
            break;

        case 'a' ... 'z':  // keyword inside preprocessor
            pevent_data.data[event_data_idx++] = ch;
            break;

        case '0' ... '9':  // number inside preprocessor
            pevent_data.data[event_data_idx++] = ch;
            break;

        default:      // remaining
            pevent_data.data[event_data_idx++] = ch;
            break;
    }
    return NULL;
}

pevent_t *pstate_header_file_handler(FILE* fd, int ch)
{
    switch(ch)
    {
        case '>':
            set_parser_event(PSTATE_PREPROCESSOR_DIRECTIVE, PEVENT_HEADER_FILE);
            state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;
            return &pevent_data;

        case '\"':
            pevent_data.data[event_data_idx++] = ch;
            set_parser_event(PSTATE_PREPROCESSOR_DIRECTIVE, PEVENT_HEADER_FILE);
            state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;
            return &pevent_data;

        case '\n':
            set_parser_event(PSTATE_IDLE, PEVENT_HEADER_FILE);
            return &pevent_data;

        default:    // collects header file name characters
            pevent_data.data[event_data_idx++] = ch;
            break;
    }
    return NULL;
}

pevent_t * pstate_reserve_keyword_handler(FILE *fd, int ch)
{
    int result;

    switch(ch)
    {
        case ' ':
        case ')':
        case '(':
        case ';':
        case ',':
        case '=':
        case '{':
        case '}':
        case '\n':
            ungetc(ch, fd);     // putting ch back
            
            word[word_idx] = '\0';
            word_idx = 0;
            result = is_reserved_keyword(word);

            if(result)
            {
                pevent_data.property = result;   // RES_KEYWORD_DATA or RES_KEYWORD_NON_DATA
                set_parser_event(PSTATE_IDLE, PEVENT_RESERVE_KEYWORD);
            }
            else    // not a keyword just identifier
            {
                set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
            }
            return &pevent_data;
        
        default:
            word[word_idx++] = ch;
            pevent_data.data[event_data_idx++] = ch;
            break;
    }
    return NULL;
}

pevent_t* pstate_numeric_constant_handler(FILE* fd, int ch)
{
    switch(ch)
    {
        case ' ':
        case ';':
        case ')':
        case '}':
        case ']':
        case ',':
        case '\n':
            ungetc(ch, fd);  // putting back characters            
            set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT);
            return &pevent_data;

        default:
            pevent_data.data[event_data_idx++] = ch;
            break;
    }
    return NULL;
}

pevent_t* pstate_string_handler(FILE* fd, int ch)
{
    switch(ch)
    {
        case '\"':
            pevent_data.data[event_data_idx++] = ch;
            set_parser_event(PSTATE_IDLE, PEVENT_STRING);
            return &pevent_data;

        case '\n':
            set_parser_event(PSTATE_IDLE, PEVENT_STRING);
            return &pevent_data;

        default:
            pevent_data.data[event_data_idx++] = ch;
            break;
    }
    return NULL;
}

pevent_t * pstate_single_line_comment_handler(FILE *fd, int ch)
{
	int pre_ch;
	switch(ch)
	{
		case '\n' : /* single line comment ends here */
#ifdef DEBUG	
			printf("\nSingle line comment end\n");
#endif
			pre_ch = ch;
			pevent_data.data[event_data_idx++] = ch;
			set_parser_event(PSTATE_IDLE, PEVENT_SINGLE_LINE_COMMENT);
			return &pevent_data;
		default :  // collect single line comment chars
			pevent_data.data[event_data_idx++] = ch;
			break;
	}

	return NULL;
}
pevent_t * pstate_multi_line_comment_handler(FILE *fd, int ch)
{
	int pre_ch;
	switch(ch)
	{
		case '*' : /* comment might end here */
			pre_ch = ch;
			pevent_data.data[event_data_idx++] = ch;
			if((ch = fgetc(fd)) == '/')
			{
#ifdef DEBUG	
				printf("\nMulti line comment End : */\n");
#endif
				pre_ch = ch;
				pevent_data.data[event_data_idx++] = ch;
				set_parser_event(PSTATE_IDLE, PEVENT_MULTI_LINE_COMMENT);
				return &pevent_data;
			}
			else // multi line comment string still continued
			{
				pevent_data.data[event_data_idx++] = ch;
			}
			break;
		case '/' :
			/* go back by two steps and read previous char */
			fseek(fd, -2L, SEEK_CUR); // move two steps back
			pre_ch = fgetc(fd); // read a char
			fgetc(fd); // to come back to current offset

			pevent_data.data[event_data_idx++] = ch;
			if(pre_ch == '*')
			{
				set_parser_event(PSTATE_IDLE, PEVENT_MULTI_LINE_COMMENT);
				return &pevent_data;
			}
			break;
		default :  // collect multi-line comment chars
			pevent_data.data[event_data_idx++] = ch;
			break;
	}

	return NULL;
}

pevent_t* pstate_ascii_char_handler(FILE* fd, int ch)
{
    switch(ch)
    {
        case '\'':
            pevent_data.data[event_data_idx++] = ch;
            set_parser_event(PSTATE_IDLE, PEVENT_ASCII_CHAR);
            return &pevent_data;

        default:
            pevent_data.data[event_data_idx++] = ch;
            break;
    }
    return NULL;
}
