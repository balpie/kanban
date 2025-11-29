#include <stdio.h>
void to_upper_case(char* str) 
{
    for(char* ptr = str; *ptr != '\0'; ptr++)
    {
        if(*ptr >= 'a' && *ptr <= 'z')
        {
            printf("%c - %d = ", *ptr, 'a'-'A');
            *ptr -= 'a' - 'A';
            printf("%c\n", *ptr);
        }
    }
}

int main()
{
    char* str = "create_card";
    to_upper_case(str);
    printf("%s\n", str);
    return 0;
}
