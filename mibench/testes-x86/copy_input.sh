#!/bin/bash

MIBENCH=$(pwd)/..

mkdir -p input &&
cp $MIBENCH/automotive/susan/input_large.pgm input/input_susan.pgm &&
cp $MIBENCH/consumer/lame/large.wav input/large.wav &&
cp $MIBENCH/network/dijkstra/input.dat input/dijkstra.dat &&
cp $MIBENCH/network/patricia/large.udp input/large.udp &&
cp $MIBENCH/security/sha/input_large.asc input/input_sha.asc &&
cp $MIBENCH/security/rijndael/input_large.asc input/input_rijndael.asc &&
cp $MIBENCH/security/rijndael/input_large.enc input/input_rijndael.enc &&
cp $MIBENCH/security/blowfish/input_large.asc input/input_blowfish.asc &&
cp $MIBENCH/security/blowfish/input_large.enc input/input_blowfish.enc &&
cp $MIBENCH/telecomm/adpcm/data/large.adpcm input/large.adpcm &&
cp $MIBENCH/telecomm/adpcm/data/large.pcm input/large.pcm &&
cp input/large.pcm input/input.crc ||
{ echo "Input copy failed!"; exit 1; }
