#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define ROW_SIZE 8
#define COL_SIZE 500

int main(void)
{
    char file[] = "urlList.txt";
    char urlList[ROW_SIZE][COL_SIZE];

    FILE* filePtr;

    if((filePtr = fopen(file, "r")) == NULL){
        printf("%s%s\n", file, " failed to open.");
        return 0;
    }
    else{
        char separators[] = ",";
        int urlIndex = 0;
        char lineBuffer[COL_SIZE] = {0};

        while(fgets(lineBuffer, sizeof(lineBuffer), filePtr) != NULL)
        {
            char* token = strtok(lineBuffer, separators);
            while(token != NULL)
            {
                strcpy(urlList[urlIndex], token);
                urlIndex++;

                token = strtok(NULL, separators);
            }
        }
    }

    int numDblQuotes = 0;
    for(int index = 0; index <= COL_SIZE; index++){
        if(urlList[0][index] == '"'){
            numDblQuotes++;
        }
        if(numDblQuotes > 1){
            printf("\nNumber of elements is: %d\n", index);
            break;
        }
        printf("%c", urlList[0][index]);
    }
}