# Simple Webserver

A simple C++ based WebServer with Pool Threads and HTTP Processing.

> The webserver supports only `HTTP/1.0`, `HTTP/1.1` and basic `GET` requests. A more complete implementation is planned for the future.

## Usage

Run the following commands in the terminal to run the server:
```console
make
./server
```
The server will start listening on port `8080`. You can access it by navigating to `http://localhost:8080` in your web browser.

## HLS test generation
 - To create a sample HLS stream (playlist + .ts segments) for testing, run:

```bash
./scripts/generate_test_hls.sh 30
```

This requires `ffmpeg`. The script writes `sample.m3u8` and `.ts` segments into `www/media/`.

After generating the test HLS stream, you can access the HLS test page at `http://localhost:8080/hls.html`.
