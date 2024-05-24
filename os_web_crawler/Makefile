all: build

build:
	@echo "Building alternate code"
	gcc crawlerMT.c -lpthread -lcurl -o multiThreadCrawler
	./multiThreadCrawler

clean:
	@echo "Cleaning alt"
	rm singleThreadCrawler
