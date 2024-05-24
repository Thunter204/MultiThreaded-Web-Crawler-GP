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
#define QUEUE_URL_LIMIT 2000
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
int parseHTMLBody(char* HTMLBodyStr, char* URLsDiscovered[]); // Takes the given URL's HTML body, returns all found URLs from HTML body
int getURLFromLine(char* line, char* query, char* URLSubList[]);

int main(void){
	// char urlRU[] = "https://www.rutgers.edu"; // Used for testing
	char file[] = "/home/peter/Documents/School/Rutgers/Spring_23/OS/web_crawler/_git-repo/os_web_crawler/urlList.txt"; // Seed list file name
    char urlList[ROW_SIZE][COL_SIZE]; // 2-D array for url seed list
    char* URLQueue[QUEUE_URL_LIMIT];
    int queueHead = 0;
    int queueTail = 0; // Tail can never be larger than QUEUE_URL_LIMIT

	/* File open, read, and save seed list */
    /**/ // Only block comment the file handling out IFF working in CLion, also, replace trimmed URL below
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
    /**/ // End of CLion block comment (please leave this comment for marking purposes)

	/* curl init section, will assign HTML body to struct member */
	CURL* curl;
	CURLcode result; // Currently unused, need to know how to handle CURLCodes

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
         * This section enqueues the seed URL and repositions the head pointer
         */
        URLQueue[queueTail] = (char*)malloc(strlen(trimmedURL) * sizeof(char)); // Need to allocate based on size of URL
        strcpy(URLQueue[queueTail], trimmedURL); // Copies trimmedURL into URLQueue
        queueTail++;
        /**************************************************************************************************************/

		/***************************************************************************************************************
		 * This section will use the previously trimmed URLs and send it to curl, getting back the HTML body in the
		 * structure member (char*)
		 * The while loop iterates on the URLQueue, so long as the head pointer is in a valid position
		 **************************************************************************************************************/

        while(queueHead < QUEUE_URL_LIMIT && queueHead < queueTail)
        {
            curl = curl_easy_init(); // curl init
            if (curl) { // Initialization of <curl> returns T/F value
                struct htmlBody currBody; // Initializes a struct:htmlBody
                init_htmlBody(&currBody); // Allocates memory for the structure variable

                /* This section does not have any error handling, if we do not handle a bad URL, the logic will fail. */
                printf("################### %s ###################\n", URLQueue[queueHead]);
                curl_easy_setopt(curl, CURLOPT_URL, URLQueue[queueHead]); // <trimmedURL> needs to be used here
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunct); // Sets up the write function for write data
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &currBody); // Writes the HTML body to currBody
                result = curl_easy_perform(curl); // Executes all "set" operations ("setopt")
                queueHead++;

                /* This print is used for testing purposes...comment it out if you get tired of looking at HTML bodies */
                // printf("%s\n", currBody.bodyStrPtr);
                char *returnedURLList[PER_PAGE_URL_LIMIT]; // Don't forget to free(returnedURLList)
                printf("################### URLs found from above URL ###################\n");
                int numURLsFound = parseHTMLBody(currBody.bodyStrPtr, returnedURLList); // Call to internal function

                for (int foundURLIndex = 0; foundURLIndex < numURLsFound; foundURLIndex++) { // Copying all URLs into the URLQueue
                    URLQueue[queueTail] = (char *) malloc(strlen(returnedURLList[foundURLIndex]) * sizeof(char)); // Allocate that index to size of URL string
                    strcpy(URLQueue[queueTail], returnedURLList[foundURLIndex]); // "Enqueue" URL
                    queueTail++; // Don't forget to check this variable is not exceeding the QUEUE_URL_LIMIT
                    free(returnedURLList[foundURLIndex]);
                }

                free(currBody.bodyStrPtr); // Frees the allocated memory for the struct
            }
        }
	}
	return 0;
}

int crawlPage(char urlToCrawl[])
{
    // pass
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
int parseHTMLBody(char* HTMLBodyStr, char* URLsDiscovered[])
{
    char* savePtrLine;
    bool isBody = false;

	char* separator = "\n"; // This separator needs to be a char*...passing '\n' does not work, nor should "\n" (DIRECTLY)
	char* token = strtok_r(HTMLBodyStr, separator, &savePtrLine); // Tokenizes the string on the appropriate character "\n"

	char* http = "http"; // Used as a placeholder for "http"
    int numURLsFromBody = 0;

	while(token != NULL) // This prints out the entire HTML body line-by-line
	{
        if(strstr(token, "<body")){
            isBody = true;
        }
        if(strstr(token, "</body")){
            isBody = false;
        }
		if(strstr(token, "http")){
            int numURLsFromLine = getURLFromLine(token, http, URLsDiscovered);
			if((numURLsFromBody = numURLsFromBody + numURLsFromLine) >= PER_PAGE_URL_LIMIT){
                break;
            }
		}
		token = strtok_r(NULL, separator, &savePtrLine);
	}
    return numURLsFromBody;
}

int getURLFromLine(char* line, char* query, char* URLSubList[])
{
    char* angleBracketSep = ">"; // Used for the first line separation
    char* dblQuoteSep = "\""; // Used for the second
    char* spaceSep = " "; // The third

    char* savePtrAngle; // Re-entrant token pointer
    char* savePtrHTTP; // Re-entrant token pointer
    char* savePtrSpace; // Re-entrant token pointer
    int count = 0;

	char* angleToken = strtok_r(line, angleBracketSep, &savePtrAngle); // Tokenizes line with (>) as "angleToken"
	while(angleToken != NULL)
	{
        if(count >= PER_PAGE_URL_LIMIT){ // Somehow this isn't firing??
            printf("%s\n", "End of URLs allowed.");
            break;
        }

		if(strstr(angleToken, query)){ // If angleToken contains "http"
            char* queryToken = strtok_r(angleToken, dblQuoteSep, &savePtrHTTP); // Tokenizes angleToken with (") as "queryToken"
            while(queryToken != NULL)
            {
                if(strstr(queryToken, query)){ // If the "queryToken" contains "http"
                    char* spaceToken = strtok_r(queryToken, spaceSep, &savePtrSpace); // Tokenizes queryToken with ( ) as "spaceToken"
                    while(spaceToken != NULL)
                    {
                        if(strstr(spaceToken, query)){ // If the "spaceToken" contains "http"
                            if(spaceToken[0] != '.') {
                                URLSubList[count] = (char *) malloc(strlen(spaceToken) * sizeof(char)); // Allocates just enough space
                                strcpy(URLSubList[count], spaceToken); // Copies URL into URL array
                                printf("%s\n", URLSubList[count]); // Right now we print this, but this is where the assignment will occur
                                if (++count >= PER_PAGE_URL_LIMIT) {
                                    // printf("%s\n", "End of URLs allowed.");
                                    break;
                                }
                            }
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

	return count;
}