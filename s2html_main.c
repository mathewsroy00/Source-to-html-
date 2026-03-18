#include <stdio.h>
#include "s2html_event.h"
#include "s2html_conv.h"

int main(int argc, char* argv[])
{
    pevent_t *event;

    if(argc < 2)
    {
        printf("\nError ! please enter file name and mode\n");
        printf("Usage: <executable> <filename> \n");
        printf("Example : ./a.out abc.txt\n\n");
        return 1;
    }

#ifdef DEBUG
printf("File to be opened : %s\n", argv[1]);
#endif

    // Opening the source file
    FILE* sfp = fopen(argv[1], "r");
    if(sfp == NULL)
    {
        printf("Error! File %s could not be opened\n", argv[1]);
        return 2;
    }

    char dest_file[100];

    // check for output file name
    if(argc > 2)
        sprintf(dest_file, "%s.html", argv[2]);
    else
        sprintf(dest_file, "%s.html", argv[1]);

    // opening destination file
    FILE* dfp = fopen(dest_file, "w");
    if(dfp == NULL)
    {
        printf("Error! could not create %s output file\n", dest_file);
        return 3;
    }

    html_begin(dfp, HTML_OPEN);        // for writing html starting tags

    // read from the source file, convert to html and write to dest file
    do
    {
        event = get_parser_event(sfp);

        source_to_html(dfp, event);
    }while(event -> type != PEVENT_EOF);

    html_end(dfp, HTML_CLOSE);      // for writing html closing tags

    printf("\nOutput file %s generated\n", dest_file);

    // closing files
    fclose(sfp);
    fclose(dfp);

    return 0;
}