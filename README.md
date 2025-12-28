# Simple Webserver

A simple C++ based WebServer with Pool Threads and HTTP Processing.

> The webserver supports only `HTTP/1.0` and basic `GET` requests. A more complete implementation is planned for the future.

## Usage

To use the server on your own system set the following macros in `http_server.hh`:


Finally run the following commands in the terminal to run the server:
```console
make
./server
```
## HLS test generation
 - To create a sample HLS stream (playlist + .ts segments) for testing, run:

```bash
./scripts/generate_test_hls.sh 30
```

This requires `ffmpeg`. The script writes `sample.m3u8` and `.ts` segments into `www/media/`.
