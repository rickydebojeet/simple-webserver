#!/usr/bin/env bash
set -euo pipefail

# Usage: ./scripts/generate_test_hls.sh [duration_seconds]
# Requires ffmpeg installed. Generates HLS files into www/media

DURATION=${1:-30}
OUT_DIR="www/media"
mkdir -p "$OUT_DIR"

# Generate test HLS (video + audio) using libx264 + aac
ffmpeg -y \
  -f lavfi -i testsrc=size=1280x720:rate=25 \
  -f lavfi -i sine=frequency=1000:sample_rate=44100 \
  -t "$DURATION" \
  -c:v libx264 -preset veryfast -tune zerolatency -g 50 -sc_threshold 0 -b:v 800k \
  -profile:v baseline -level 3.0 -pix_fmt yuv420p \
  -c:a aac -b:a 128k -ac 2 \
  -f hls \
  -hls_time 4 \
  -hls_playlist_type vod \
  -hls_segment_filename "$OUT_DIR/segment%03d.ts" \
  "$OUT_DIR/sample.m3u8"

echo "HLS files written to $OUT_DIR"
