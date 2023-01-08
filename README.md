# Simple Webserver

A simple C++ based WebServer with Pool Threads and HTTP Processing.

> The webserver supports only `HTTP/1.0` and basic `GET` requests.

## Usage

To use the server on your own system set the following macros in `http_server.hh`:

- `HEADER_MAX`: Maximum size of the header. (Default: 8192)
- `THREAD_MAX`: The number of threads in the Pool Threads. (Default: 32)
- `PORT`: The port to use for the webserver. (Default: 8080)
- `QUEUE_MAX`: The maximum size of the queues. (Default: 64)

Finally run the following commands in the terminal to run the server:
```console
make
./server
```
