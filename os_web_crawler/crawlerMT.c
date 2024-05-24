/* Compiler Directives */
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h> // Used for explicit boolean checks
#include<string.h> // Parser heavily relies on this, so does the file parser
#include<curl/curl.h> // 3rd party library needed to get the HTML body for any given URL
#include<pthread.h>

/* Different global definitions */
#define ROW_SIZE 10
#define COL_SIZE 500
#define PER_PAGE_URL_LIMIT 10
#define QUEUE_URL_LIMIT 2000
#define DEF_STR "\033[0;0m" // Resets string color
#define BLUE_STR "\033[0;34m" // Sets string color to blue
#define RED_STR "\033[0;31m" // '''' to red
#define GREEN_STR "\033[0;32m" // ''''to green


//handles thread synchronization, ensures that only one thread
//at a time can execute a specific piece of code
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;


/**************************************************************************
 * STRUCT HTML (Stores the information from the curl function that
                                      gets the HTML body)
                                      
 * -> The literal HTML body as a char*
 * -> Length of the HTML body
 **************************************************************************/
struct htmlBody{
	char* bodyStrPtr;
	size_t lenBody;
};

/******************************************************************
* STRUCTURE THREAD(Used to pass data to thread function for crawl)
*******************************************************************/
struct thread_data {
    char** urlToCrawl;
    int* headOfQueue;
    int* tailOfQueue;
};


/*************************************************************************
 * FUNCTION PROTOTYPES
 *************************************************************************/
int crawlPage(char** urlToCrawl, int* headOfQueue, int* tailOfQueue);
int parseHTMLBody(char* HTMLBodyStr, char* URLsDiscovered[]); // Takes the given URL's HTML body, returns all found URLs from HTML body
int getURLFromLine(char* line, char* query, char* URLSubList[]);
void init_htmlBody(struct htmlBody* body);
void *threadFunc(void* threadArgs);
size_t writeFunct(void* ptr, size_t size, size_t nMemb, struct htmlBody* body);


/*************************************************************************
 * MAIN
 * opens file for reading that contains a list of URLS, seperates URLs
 * in file, trims them and adds them to queue for crawling. pthreads are
 * created for each URL in the file which are used with other arguments
 * to be able to implement multithreaded crawling.
 *************************************************************************/
int main(void){
    curl_global_init(CURL_GLOBAL_ALL);
 //   char urlRU[] = "https://www.rutgers.edu"; // Used for testing
    char file[] = "urlList.txt"; // Seed list file name
    char urlList[ROW_SIZE][COL_SIZE]; // 2-D array for url seed list
    char* URLQueue[QUEUE_URL_LIMIT];

    pthread_t threadIds[ROW_SIZE]; //stores thread IDs

    struct thread_data threadArgs[ROW_SIZE];
    int numOfThreads = 0; //threads created


    int queueHead = 0;
    int queueTail = 0; // Tail can never be larger than QUEUE_URL_LIMIT

	/*File open, read, and save seed list */
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
    fclose(filePtr);
     // End of CLion block comment (please leave this comment for marking purposes)

     /**
     apprently the number of threads will only print if we use an integer,
     I tried doing something like file[4] but it wouldn't print the number
     of threads
     **/

	for(int index = 0; index < 1; index++) // For loop used to loop over the seed list
	{
		/* The following section is needed to trim the string containing the URL, since curl will try to parse any left-over chars
		 -->We should keep this in case we need to bust this out into a function and trim URLs we get*/
		char trimmedURL[COL_SIZE] = {0}; // Initializes trimmedURL array
		int quotesFound = 0; // Initializes num of quotes found, max 2
		
		FILE* CrawlerPtr = fopen("Crawl.txt", "a");
       		if (CrawlerPtr == NULL){
           		printf("Failed to open file\n");
       		}

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

        // This section enqueues the seed URL and repositions the head pointer
        pthread_mutex_lock(&queue_mutex);
        URLQueue[queueTail] = (char*)malloc(strlen(trimmedURL) * sizeof(char)); // Need to allocate based on size of URL
        strcpy(URLQueue[queueTail], trimmedURL); // Copies trimmedURL into URLQueue
	fprintf(CrawlerPtr, "THREAD URL: %s \n", URLQueue[queueTail]);
        queueTail++;
        pthread_mutex_unlock(&queue_mutex);

       /* while(queueHead < QUEUE_URL_LIMIT && queueHead < queueTail)
        {
            int* headQPtr = &queueHead;
            int* tailQPtr = &queueTail;
            int retCrawl = crawlPage(URLQueue, headQPtr, tailQPtr);
            if(retCrawl == 1){
                // print something here?
            }
        }*/

        for (int i=0; i < queueTail; i++){
            threadArgs[i].headOfQueue = &queueHead;
            threadArgs[i].tailOfQueue = &queueTail;
            threadArgs[i].urlToCrawl = URLQueue;

            pthread_create(&threadIds[i], NULL, threadFunc, (void*)&threadArgs[i]);
            numOfThreads++;
        }
    fprintf(CrawlerPtr,"THE NUMBER OF THREADS: %d \n", numOfThreads);
    fclose(CrawlerPtr);
    }
    printf("!!!!!!!!!!!NUMBER OF THREADS: %d: \n", numOfThreads);

    //wait for thread to terminate
    for (int i = 0; i < numOfThreads; i++) {
        pthread_join(threadIds[i], NULL);
    }
	curl_global_cleanup();
	return 0;
}

 /*********************************************************************
 * This function defines the behavior of each thread in the code.
 * takes thread_data struct as an argument and loops through URL
 * queue, calling crawlPage on each URL. Thread exits when finished.
 * basically makes it possible to make threads using int and char
 * pointers
 ************************************************************************/
void *threadFunc(void* threadArgs)
{

    struct thread_data* td = (struct thread_data*)threadArgs;

    while (*td->headOfQueue < *td->tailOfQueue) {
        crawlPage(td->urlToCrawl, td->headOfQueue, td->tailOfQueue);
    }

    pthread_exit(NULL);
}


 /*********************************************************************
 * This function crawls through URLs by using the HTML code from given
 * URL, which is parsed for the URLS it contains. those URLs are added
 * to queue for more crawling.
 ************************************************************************/
int crawlPage(char** urlToCrawl, int* headOfQueue, int* tailOfQueue)
{
    /* curl init section, will assign HTML body to struct member */
    CURL* curl;
    CURLcode result; // Currently unused, need to know how to handle CURLCodes

    FILE* CrawlerPtr = fopen("Crawl.txt", "a");
        if (CrawlerPtr == NULL){
            printf("Failed to open file\n");
        }

    curl = curl_easy_init(); // curl init
    if (curl) { // Initialization of <curl> returns T/F value
        struct htmlBody currBody; // Initializes a struct:htmlBody
        init_htmlBody(&currBody); // Allocates memory for the structure variable


        /* This section does not have any error handling, if we do not handle a bad URL, the logic will fail. */
        pthread_mutex_lock(&queue_mutex);
        printf("\nURL:  %s \n", urlToCrawl[(*headOfQueue)]);
        curl_easy_setopt(curl, CURLOPT_URL, urlToCrawl[(*headOfQueue)]); // <trimmedURL> needs to be used here
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunct); // Sets up the write function for write data
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &currBody); // Writes the HTML body to currBody
        result = curl_easy_perform(curl); // Executes all "set" operations ("setopt")
        (*headOfQueue)++;
        pthread_mutex_unlock(&queue_mutex);

        /* This print is used for testing purposes...comment it out if you get tired of looking at HTML bodies */
        // printf("%s\n", currBody.bodyStrPtr);
        if(result == CURLE_OK){
            //long resCode;
            //curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resCode);

            //if(resCode == 200){
                char *returnedURLList[PER_PAGE_URL_LIMIT]; // Don't forget to free(returnedURLList)
		fprintf(CrawlerPtr, "URLs found from above URL: \n");
                printf("################### URLs found from above URL ###################\n");
                int numURLsFound = parseHTMLBody(currBody.bodyStrPtr, returnedURLList); // Call to internal function

                for (int foundURLIndex = 0; foundURLIndex < numURLsFound; foundURLIndex++) { // Copying all URLs into the URLQueue
                urlToCrawl[(*tailOfQueue)] = (char *) malloc(strlen(returnedURLList[foundURLIndex]) * sizeof(char)); // Allocate that index to size of URL string
                strcpy(urlToCrawl[(*tailOfQueue)], returnedURLList[foundURLIndex]); // "Enqueue" URL
                (*tailOfQueue)++; // Don't forget to check this variable is not exceeding the QUEUE_URL_LIMIT
		fprintf(CrawlerPtr, "%s\n", returnedURLList[foundURLIndex]);
                free(returnedURLList[foundURLIndex]); // [REQUIRED] iff allocating, also freeing memory
            //}
        }
        }
        else{
            printf("~~~~~~~~~~~~~~~~~~~ Bad URL -- skipping ~~~~~~~~~~~~~~~~~~~\n");
        }
	fclose(CrawlerPtr);
        free(currBody.bodyStrPtr); // [REQUIRED] frees the allocated memory for the struct
        return 1; // For SUCCESS
    }
    return 0; // For FAILURE
}

/************************************************************************
 * This function initializes the HTML body, which points to html body
 * struct. this functions allocates space for the hmtlbody
 ************************************************************************/
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

/************************************************************************
 * This function calculates the size of data that needs to be added to
 * HTML body (checks if theres enough memory to allocate), then copies
 * data to the memory block starting at newlen (null terminator is added
 * at the end). lenBody is updated
 ************************************************************************/
size_t writeFunct(void* ptr, size_t size, size_t nMemb, struct htmlBody* body)
/*ptr: ptr to data to be written, size ofelement written, number of elements written, ptr to strucutre that contains HTML info*/
{
    /*newlen(size of new data that needs to be added to HTML body) =
                    current HTML_bdylen + size of data to be written * number of elements to be written */
	size_t newLen = body->lenBody + size * nMemb;
	body->bodyStrPtr = realloc(body->bodyStrPtr, newLen + 1);

	/*Checks if there is enough memory*/
	if(body->bodyStrPtr == NULL){
		fprintf(stderr, "failed to allocate more memory\n");
		exit(EXIT_FAILURE);
	}
    /*Data is copied to memory block starting at current HTML_bdylen
      Null terminator is added to end of newlen
      updated lenBody is updated*/
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

/********************************************************************************************************
 * Tokenizes line using three delimiters '>, ", __space__' and searches for URLs that match the query.
 * Once found, memory is allocated fo rit and is coppied to URLSubList array and printed to terminal.
 *********************************************************************************************************/
int getURLFromLine(char* line, char* query, char* URLSubList[])
/*line = ptr to a char str that contains iput line to be parsed, query= ptr to char str; to be searched,
  URLSubList = ptr to char str that stores th elist of URLs found*/
{
    char* angleBracketSep = ">"; // Used for the first line separation
    char* dblQuoteSep = "\""; // Used for the second
    char* spaceSep = " "; // The third

    char* savePtrAngle; // Re-entrant token pointer
    char* savePtrHTTP; // Re-entrant token pointer
    char* savePtrSpace; // Re-entrant token pointer
    int count = 0;
    bool isMaxFound = false;

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
                                URLSubList[count] = (char *) malloc((strlen(spaceToken) + 1) * sizeof(char)); // Allocates just enough space
                                strcpy(URLSubList[count], spaceToken); // Copies URL into URL array
                                printf("%s\n", URLSubList[count]); // Right now we print this, but this is where the assignment will occur
                                if (++count >= PER_PAGE_URL_LIMIT) {
                                    // printf("%s\n", "End of URLs allowed.");
					                isMaxFound = true;
                                    break;
                                }
                            }
                        }
                        spaceToken = strtok_r(NULL, spaceSep, &savePtrSpace); // Gets next token for "spaceToken"
                    }
                    // printf("%s\n", queryToken); // Used for debugging queryToken
                }
                queryToken = strtok_r(NULL, dblQuoteSep, &savePtrHTTP); // Gets next token for "queryToken"
		if(isMaxFound){
		    break;
		}
            }
			// printf("%s\n", angleToken); // Used for debugging angleToken
		}
		angleToken = strtok_r(NULL, angleBracketSep, &savePtrAngle); // Gets next token for "angleToken"
		if(isMaxFound){
		    break;
		}
	}

	return count;
}
