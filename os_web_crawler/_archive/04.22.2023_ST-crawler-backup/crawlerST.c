/* Compiler Directives */
#include<stdio.h> // boiler plate
#include<stdlib.h> // boiler plate
#include<stdbool.h> // Used for explicit boolean checks
#include<string.h> // Parser heavily relies on this, so does the file parser
#include<curl/curl.h> // 3rd party library needed to get the HTML body for any given URL

/* Different global definitions */
#define ROW_SIZE 10
#define COL_SIZE 500
#define PER_PAGE_URL_LIMIT 10
#define DEF_STR "\033[0;0m" // Resets string color
#define BLUE_STR "\033[0;34m" // Sets string color to blue
#define RED_STR "\033[0;31m" // '''' to red
#define GREEN_STR "\033[0;32m" // ''''to green

/* Structure definition */
struct htmlBody{ // Stores the information from the curl function that gets the HTML body
	char* bodyStrPtr; // The literal HTML body as a char*
	size_t lenBody; // Length of the HTML body
};

/* Function Prototypes */
void init_htmlBody(struct htmlBody* body); // Used to initialize the structure
size_t writeFunct(void* ptr, size_t size, size_t nMemb, struct htmlBody* body); // 
char** parseHTMLBody(char* HTMLBodyStr); // Takes the given URL's HTML body, returns all found URLs from HTML body
char* getURLFromLine(char* line, char* query, char** URLSubList);

int main(void){
	char urlRU[] = "https://www.rutgers.edu"; // Used for testing
	char file[] = "urlList.txt"; // Seed list file name
    char urlList[ROW_SIZE][COL_SIZE]; // 2-D array for url seed list

	/* File open, read, and save seed list */
    /* // Only block comment the file handling out IFF working in CLion, also, replace trimmed URL below
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
    */ // End of CLion block comment (please leave this comment for marking purposes)

	// The following line can be removed
	// printf("%s\n%s\n%s\n%s\n", urlList[0], urlList[1], urlList[2], urlList[3]); // Used for testing

	/* curl init section, will assign HTML body to struct member */
	CURL* curl;
	CURLcode result;

	for(int index = 0; index < 1; index++) // For loop used to loop over the seed list
	{
		/***************************************************************************************************************
		 * The following section is needed to trim the string containing the URL, since curl will try to parse any left-over chars
		 * ==> We should keep this in case we need to bust this out into a function and trim URLs we get
		 **************************************************************************************************************/
		char trimmedURL[COL_SIZE] = {0}; // Initializes trimmedURL array
		int quotesFound = 0; // Initializes num of quotes found, max 2
		for(int subStringLen = 0; subStringLen < (COL_SIZE - 2); subStringLen++)
		{
			if(urlList[index][subStringLen] == '"'){ // IFF char is double quotes
				if(++quotesFound > 1){ // IFF this is the second double quotes
					trimmedURL[subStringLen - 1] = '\0'; // Adds the null terminator to the character string
					break; // Escape the for loop
				}
			}
			else{ // IFF char is not double quotes...
				trimmedURL[subStringLen - 1] = urlList[index][subStringLen]; // Assigns URL character to array string
			}
		}
		// printf("%s", trimmedURL); // This was for proving I trimmed the URL successfully
        /**************************************************************************************************************/

		/***************************************************************************************************************
		 * This section will use the previously trimmed URLs and send it to curl, getting back the HTML body in the
		 * structure member (char*)
		 **************************************************************************************************************/
		curl = curl_easy_init(); // curl init
		if(curl){ // init returns T/F value
			struct htmlBody currBody;
			init_htmlBody(&currBody);
			/* This section does not have any error handling, if we do not handle a bad URL, the logic will fail. */
			curl_easy_setopt(curl, CURLOPT_URL, urlRU); // <trimmedURL> needs to be used here
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunct); // Sets up the write function for write data
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &currBody); // Writes the HTML body to currBody
			result = curl_easy_perform(curl); // Executes all "set" operations ("setopt")

			/* This print is used for testing purposes...comment it out if you get tired of looking at HTML bodies */
			// printf("%s\n", currBody.bodyStrPtr);
			char** URLsFromHTML = parseHTMLBody(currBody.bodyStrPtr);
			free(currBody.bodyStrPtr); // Frees the allocated memory for the struct
		}
	}
    // printf("Am I exiting?\n");
	return 0;
}

void init_htmlBody(struct htmlBody* body)
{
	body->lenBody = 0;
	body->bodyStrPtr = malloc(body->lenBody + 1);

	if(body->bodyStrPtr == NULL){
		fprintf(stderr, "failed to allocate memory\n");
		exit(EXIT_FAILURE);
	}
	body->bodyStrPtr[0] = '\0';
}

size_t writeFunct(void* ptr, size_t size, size_t nMemb, struct htmlBody* body)
{
	size_t newLen = body->lenBody + size * nMemb;
	body->bodyStrPtr = realloc(body->bodyStrPtr, newLen + 1);
	if(body->bodyStrPtr == NULL){
		fprintf(stderr, "failed to allocate more memory\n");
		exit(EXIT_FAILURE);
	}
	memcpy(body->bodyStrPtr + body->lenBody, ptr, size * nMemb);
	body->bodyStrPtr[newLen] = '\0';
	body->lenBody = newLen;

	return size * nMemb;
}

/***********************************************************************************************************************
 * This will take the HTML body, go over the HTML body line-by-line looking for other URLs.
 * -> We may want to use parseurl (https://curl.se/libcurl/c/parseurl.html)
 * --> This may help with complicated URLs found during the crawl, we can use this to focus the crawl on main pages,
 * rather than potentially unknown links (like download URLs, .pdf URLs, etc)
 **********************************************************************************************************************/
char** parseHTMLBody(char* HTMLBodyStr)
{
	/*
	We can work with this easily
	All we need to do here is to test tokenizing the string on '\n' and seeing if we can traverse it that way
	IFF we can, then this step should be really easy
	B/c then all we have to do is implement the strstr() function from string.h
	The part where we extract the explicit URL, however, will be a bit more difficult
	This will require... (at least)
	-> Tokenizing the string in question on some HTML syntactical pattern (<), (>), (href=), etc
	-> Then, once the string has been split into smaller chunks, find the start of the URL "http"
	--> We should not focus on https, since looking for http will find https, but not the other way around, necessarily
	-> If a URL is linked (using href), it will be enclosed in double quotes
	-> If a URL is in text, it may not
	*** I don't think we should worry too much about this, we are not trying to be insanely comprehensive in our crawl after all
	-> Once we have a FULL URL, as stated in the HTML body, possibly trim it, always save it to the search queue
	*/

    /*******************************************************************************************************************
     * The following is a demonstration of taking the HTML body character-by-character and printing it to screen.
     * It will stop once it hits the first new line character <\n>. Then <break>s out of the infinite while loop
     * Un-comment the block comments noted for this section to use this logic again
     ******************************************************************************************************************/
    /* // To allow the logic to function, remove the </*> at the beginning of this line. Replace the </*> when done
	printf("In function body:\n"); // Used for testing
	int index = 0;
	while(1)
	{
		printf("~~~\nThis is a character-by-character print\n~~~\n%c", HTMLBodyStr[index]); // Prints each character in the string, until...
		if(HTMLBodyStr[++index] == '\n'){ // ...we reach a '\n'
			puts(""); // ...Then prints a '\n'
			break; // ...Then escapes this infinite loop
		}
	}
    */ // To allow the logic to function, remove the </*> at the beginning of this line. Replace the </*> when done
    /******************************************************************************************************************/
    char* savePtrLine;

	char* separator = "\n"; // This separator needs to be a char*...passing '\n' does not work, nor should "\n" (DIRECTLY)
	// printf("~~~\n"); // Debugging
	char* token = strtok_r(HTMLBodyStr, separator, &savePtrLine); // Tokenizes the string on the appropriate character "\n"
	// printf("~~~\n"); // Debugging
	// printf("%s\n", token); // Print it

	int numURLs = 0;
	int bufferSize = PER_PAGE_URL_LIMIT;
	char** URLPtrs = malloc(bufferSize * sizeof(char*));

	char* http = "http"; // Used as a placeholder for "http"

	while(token != NULL) // This prints out the entire HTML body line-by-line
	{
		/***************************************************************************************************************
		 * Please uncomment the following code within the "if" statement only if you desire to print the lines that contain "http"
		 * IF you do this, you will NEED to comment out the printf() following the if{} statement.
		 **************************************************************************************************************/

		/**/ // delete the block quotes to allow the following... please replace this text when finished (if needed)
		if(strstr(token, "http")){
			// send to function...???
			printf("\n^^^\n~~~\n%s\n~~~\n", token);

			char* ret = getURLFromLine(token, http, URLPtrs); // Call to function that gets URL from line
		}
		/**/
		// printf("%s\n", token); // Add another double forward slash at the beginning of this line to comment out
		token = strtok_r(NULL, separator, &savePtrLine);
        // printf("%s\n", token);
	}

    return URLPtrs;
}

char* getURLFromLine(char* line, char* query, char** URLSubList)
{
	/*******************************************************************************************************************
	 * We will take a line that has already been identified as containing "http"
	 * We will then iterate through that line
	 * --THOUGHTS (about how to do this)--
	 *   -> tokenize the line first for right angle-bracket
	 *   -> then tokenize again for double-quotes
	 *   -> then do strstr() on "http"
	 *   --> this should catch a large majority of URLs on a page
	 *   --> the one issue I can forsee is getting a bad link...but is this really an issue?
	 *   --> what really happens when we pass a bad link to curl? From my experience it has not output anything
	 *   --> if we can handle this at this stage then we can "simplify" the issue
	 *   --> we can use a URL counter to "identify" a "useless" URL
	 *   ---> The implication is that if we increase the URL count then we we able to open that URL, AND it was useful
	 *   -> IFF strstr() and we got URLs return SUCCESS?
	 ******************************************************************************************************************/
	char* angleBracketSep = ">"; // Used for the first line separation
	char* httpSep = "http"; // Used for the second line separation --> "http" not needed
    char* dblQuoteSep = "\"";
    char* spaceSep = " ";

    /*******************The following only matters for the previous two initializations*********************************
     * A URL can be thought of as the following
     * <...>...<...>
     * Or a sequence of <...> and ...
     * Within these <...>, there are double quotes, that encapsulate certain objects
     * Some of these objects are URLs
     * Hence the use of tokenizing the line on right angle brackets FIRST
     * THEN checking if that token has "http"
     * IFF it does DO
     * -> tokenize that token on double quotes
     * -> THEN if that token contains "http"
     * -> Check that token for a URL ending...or assume that URL is fine, save it and continue...allowing curl to tell
     * us if the URL is bad
     ******************************************************************************************************************/
     char* savePtrAngle;
     char* savePtrHTTP;
     char* savePtrSpace;

	printf("%s\n\n", "~~~~~ Now for going inside ~~~~~");

	char* angleToken = strtok_r(line, angleBracketSep, &savePtrAngle); // Tokenizes line with (>) as "angleToken"
	while(angleToken != NULL)
	{
		if(strstr(angleToken, "http")){ // If angleToken contains "http"
            char* queryToken = strtok_r(angleToken, dblQuoteSep, &savePtrHTTP); // Tokenizes angleToken with (") as "queryToken"
            while(queryToken != NULL)
            {
                if(strstr(queryToken, query)){ // If the "queryToken" contains "http"
                    char* spaceToken = strtok_r(queryToken, spaceSep, &savePtrSpace); // Tokenizes queryToken with ( ) as "spaceToken"
                    while(spaceToken != NULL)
                    {
                        if(strstr(spaceToken, query)){ // If the "spaceToken" contains "http"
                            printf("%s\n", spaceToken); // Right now we print this, but this is were the assignment will occur
                        }
                        spaceToken = strtok_r(NULL, spaceSep, &savePtrSpace); // Gets next token for "spaceToken"
                    }
                    // printf("%s\n", queryToken); // Used for debugging queryToken
                }
                queryToken = strtok_r(NULL, dblQuoteSep, &savePtrHTTP); // Gets next token for "queryToken"
            }
			// printf("%s\n", angleToken); // Used for debugging angleToken
		}
		angleToken = strtok_r(NULL, angleBracketSep, &savePtrAngle); // Gets next token for "angleToken"
	}

	return angleBracketSep; // REMOVE, this is a placeholder
}