# Operating Systems (Rutgers U)
### Project - web crawler
##### Team members: Toni Hunter, Peter Rodeffer

___
#### OBJECTIVE:
To implement code in C, along with third party libraries, that will take in a file with various URLs
and will "open" those URLs, find more URLs, and crawl them to find more URLs, and so on.

Our scope for this project is the ability to scrape more than URLs from the HTML pages "opened".

___
#### HOW TO RUN:
-> Please note that the `Makefile` is intended to run the crawler
please use the following command in the linux terminal:
- `make`
    - This will build the source file `crawlerMT.c` as `multiThreadCrawler`, then executes it
        - To clean use `make clean`
- This build process relies on the installation of the cURL libraries, specifically
    - curl
        - Installed with `sudo apt install curl`
    - libcurl4-openssl-dev
        - Installed with `sudo apt install libcurl4-openssl-dev`
- This build also relies on the use of pthreads, but these libraries are installed already
- This means all of the previously mentioned libraries will have to be linked in `gcc`, hence the `Makefile`
    - The command for this is: `gcc crawlerMT.c -lcurl -lpthread -o multiThreadCrawler`
    - Then to run: `./multiThreadCrawler`

___
#### TO-DO:
What do we still need to do?
- Single threaded crawler
    - Take a file, open it, get URLs
    - Put those URLs into a data structure
        - OBSERVATION!!! (the "flat" structure I have made needs to be trimmed, I think, in order to 
        pass the string to the curl command in a meanignful way)
        - To be very clear. I mean that when I "grab" the URL from the file, I put that URL into
        an array that, should be, over-sized. The implication being that a minimized string needs
        to be passed to the curl functions, not a string with a bunch of garbage since curl will
        not parse that garbage out of the array (seemingly)
    - Iterate through that structure and open the first URL with the curl commands in `crawlerST.c`
    - If we can do one, we can do all
    - From that, scan through the structure's `bodyStrPtr`, line by line, finding URLs in those lines
    using `strstr()` from `#include<string.h>`
        - Once we find such a line, then parse that line for every instance of a URL, saving it
            - I have this part nearly worked out. I can tell which tokens contain what
                - All I need to do is play around with the line and tokenize it with right angle brackets (>) and double quotes (")
    - If we can do all of this, then forming it into a solution that saves found URLs to a queue as 
    well as records the URLs found from all seed URLs
    - This ended up being true. HOWEVER, I came to find out that you cannot tokenize a token using only `strtok()`
    - Rather, you need to use `strtok_r()` which uses a second pointer that enables the user to tokenize the token again
    - After that, I needed to determine the best way to hand the URLs back to `main()`. I settled on passing an array of char* through the functions
    - Once the URLs were available in `main()` it was only a little more work to set up the pseudo-queue and the loop for it
    - Running the built executable now will demonstrate the crawling ability so far.
      - This amounts to 3 URLs crawled... but it's a start

- Multi-threaded crawler
    - Supposedly this won't be terribly difficult once we have a single threaded solution
    - We will need to know how to manage the queue when we multi-thread, as well as the data structure
    saving all found URLs for a given seed url.
    - ...???

- Report file
    - Not too hard
    - Should be a single file that contains the following:
        - Summary
            - Num of seed URLs
            - Num of URLs found
              - Possibly from each seed and from each URL
                - This may be cumbersome on the system's memory (keeping track of it, that is)
            - Num of unique URLs (found - seed)
            - Avg num URLs found per seed
            - Avg num of URLs found per URL
              - This, actually, necessitates the need to keep track of # of URLs found per URL 
            - Std deviation/variance??
        - Details
            - Every seed URL
                - URL found from seed URL, tree position?
                  - "Tree position" means this...
                    - Seed URL is (`1`)
                      - Every `i`th URL found from seed is (`1`.`i`)
                        - Every `j`th URL found from every `i`th URL is (`1`.`i`.`j`)
                          - *and so on...*
            - Alternatively (**rather first, we should simply print out all URLs found from every seed line-by-line**)
              - Seed URL
                - `i`th URL found
                - `i + 1` URL found
                - `...`

___
#### Resources:
- [libcurl functions list](https://curl.se/libcurl/c/allfuncs.html)
- [Part of `crawlerST.c` that gives the HTML body as a struct char*](https://stackoverflow.com/questions/2329571/c-libcurl-get-output-into-a-string)
- [Installation of libcurl](https://everything.curl.dev/get/linux)
- [Simple liburl example](https://curl.se/libcurl/c/cimple.html)
- [libcurl's programmer tutorial](https://curl.se/libcurl/c/libcurl-tutorial.html)
- [libcurl's multi-threading example](https://curl.se/libcurl/c/multithread.html)
- [libcurl's crwaler example](https://curl.se/libcurl/c/crawler.html)

___
#### Work log:
___
- File parser fixed, now it is within main() not in its own function
    - Data structure storing those URLs is `char [variableName][][]`
- Identified that the URLs stored require "trimming"
    - This is due to the fact that the string has a lot of garbage, but it might be as simple as the double quotes encapsulating
- Sent first trimmed URL from file to curl
    - WORKED!
- Started making HTML parser function (this will break the string down by new line characters, look for URLs, store them, then return them)
    - Got the tokenizing working for the string
    - Now, if you run it, it will print out every line that contains "http" (similar to `grep`)
___
___
- I started on the function that takes a line and tokenizes it appropriately, first on right angle brackets (>), then on double quotes (")
- I didn't get terribly far, only enough to see that I can isolate things as I have been able to
- More importantly, I was able to get VS Code and CLion (IntelliJ) set-up to link curl into the compilation process
    - This means I can debug in either CLion or VS Code. Truly a game-changer
___
___
- I figured out that `strtok(<string>, <delim>)` is not sufficient for our purposes. Rather, `strtok_r(<string>, <delim>, <address to save pointer>)`
does what is needed, since `strtok()` fails when you try to tokenize one of its tokens. This has to do with `strtok()` having an internal static pointer...
blah blah blah...
- I changed all of the instances of `strtok()` to `strtok_r()` appropriately and the "engine turned over", so to speak.
- This means that the program no longer self-terminates after the first line of the nested tokenizing!!! (we movin')
- Cleaned up the output of the program, it now displays the line it has identified as having "http", then prints it to screen. Then it identifies the URLs and prints THOSE to screen
___
___
- After getting the URLs to be more isolated, I was able to pass a `char* URLArray[]` from `main()` to `getURLFromLine()` and back
- I then copy those URLs into the queue I am using to drive the crawl
- Once I was able to prove the effective copying and re-use of URLs, I was able to quickly build out the queue loop
- This now reads the URL file, saves the URLs, enqueues the first URL, opens the HTML body, gets more URLs, enqueues those URLs, moves on to the next URL in the queue,...
- I also heavily modified the block comments, it was getting hard to read.
___
___
- By request, I broke out the crawling logic into its own function `crawlPage()`
- I had a few issues with an infinite loop situation, but this was due to me not referencing the head/tail pointer-pointers appropriately. I was incrementing the pointers `headOfQueue++`, not the value `(*headOfQueue)++` and the same with `int* tailOfQueue`.
- While doing this, I updated a few comments
- I also identified a potential source of trouble. One which I identified a while ago and lost it in my sea of comments, highlighting the importance of separate documentation for trouble points or w/e.
___
___
- Big day/day-and-a-half
- pthreads is implemented in a somewhat simplistic form (I believe it is currently "simulating single-threading" by keeping the thread execution to `1`)
- I was able to overcome the `malloc()` issue by ... changing an `i` to an `i + 1` ...................*anyway*
- Because of this development I realized that I needed to handle bad URLs being passed to curl handles. I still need to check this out, it may be that I am not handling it appropriately.
    - The reason being that some of the "bad URLs" are not actual URL's, which means I need to investigate
        - Ex./ `THREAD:  /etc/ssl/certs/ca-certificates.crt`
            - While this seems to be a clickable link, it is not...*why is my logic keeping this???*
- The primary point is that we are **REALLY** crawling now
    - Albeit with an error of `corrupted size vs. prev_size`
___