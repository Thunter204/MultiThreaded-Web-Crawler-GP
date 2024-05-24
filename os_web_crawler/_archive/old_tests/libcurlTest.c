#include<stdio.h>
#include<curl/curl.h>

int main(void)
{
    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://curl.se/libcurl/c/simple.html");
        
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        fprintf(stdout, "<<DEBUGGING>>\n");

        res = curl_easy_perform(curl); // This seems to print out the page body in the stdout stream
        fprintf(stdout, "<<DEBUGGING-2>>\n");
        fprintf(stdout, "%s", curl);

        if(res != CURLE_OK){
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else{
            fprintf(stdout, "We got one...\n");
        }

        curl_easy_cleanup(curl);
    }
    return 0;
}