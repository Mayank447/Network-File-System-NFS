#include "header_files.h"

// Remove leading whitespaces
void trim(char *str) {
    int i=0, j=0, len = strlen(str);
    while(isspace((unsigned char)str[i]) && i<len) i++;
    while(i<len && !isspace((unsigned char)str[i])) str[j++] = str[i++];
    str[j] = '\0';
}