# WebServer

This is the final project of CIT 5950. Our group implemented a multi-threaded HTTP server with networking, C++ and systems programming. This web server can provide simple searching and file viewing utilities. 

## Technology
* C++, including objects and STL
* multi-threaded server
* HTTP socket API (server bind, listen, accept, read, write and close)
* POSIX socket API
* boost library

## Functionality
* read from imported files and parse those files to record any words that show up in those files
* allow connections and handle HTTP requests
* generate threads to handle the connections
* process the word requests and query requests to fetch the files that contain the word(s)
