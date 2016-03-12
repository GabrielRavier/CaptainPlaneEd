#include <stdio.h>
#include <stdlib.h>

long ReadPlain(const char *srcfile, FILE* dst, long Pointer, int length)
{
    FILE* src = fopen(srcfile, "rb");
    if(src == NULL) return -1;

    if(length == 0) {
        if(Pointer == 0) {
            fseek(src, 0, SEEK_END);
            length = ftell(src);
            rewind(src);
        } else return -1;
    }
    
    fseek(src, Pointer, SEEK_SET);
    rewind(dst);

    char* buffer = (char*) malloc(length);
    fread(buffer, length, 1, src);
    fwrite(buffer, length, 1, dst);
    //free(buffer);
    
    fclose(src);
    return length;
}

bool CheckCreateBlankFile(char *srcfile, FILE* dst, long Pointer, int length)
{
    FILE* src = fopen(srcfile, "rb");
    if(src != NULL) {
        //file does exist, don't overwrite
        fclose(src);
        return false;
    }

    //create blank file
    for(int i=0; i< Pointer + length; i++)
        fputc(0, dst);

    return true;
}
