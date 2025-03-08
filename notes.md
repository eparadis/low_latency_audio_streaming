# low latency audio streaming

The goal of this is to pipe reasonable quality audio directly over the internet
with as little latency as possible.

The application is playing music live, so ultimately we are concerned with the
round-trip latency.

Stereo audio is preferrable, but monophonic sound is acceptable.

Loss is acceptible if the stream recovers quickly.

# Estimates

Music played at 120 beats per minute has

```
120 beats    1 minute    =  2 beats   =    1 beat
  1 minute  60 seconds      1 second     500 mS
```

The great circle distance from Seattle, WA to Burlington, VT (SEA to BTV) is
2333 miles or 3755 kilometers.

SEA lat,lon is 47.4490013, -122.3089981
BTV lat,lon is 44.4719009399414, -73.1532974243164

Lightspeed distance of 3755 kilometers is 12.5 mS, resulting in a round-trip
time of 25 mS.


# Software

## FFMPEG
[FFMPEG on MacOS](https://apple.stackexchange.com/questions/326388/terminal-command-to-record-audio-through-macbook-microphone/326390#326390)

I installed `ffmpeg` with `brew install ffmpeg`. This installed version 7.1.1.

The command `ffmpeg -f avfoundation -list_devices true -i ""` will list MacOS
audio devices, but it also errors at the end for some reason.

Explanation
```
 -f fmt           force input or output file format
 -i url           input file URL
 -list_devices    ??? perhaps an option for the 'avfoundation' "format"?
 ""               this is where the output filename would go
```

Recording audio to a file
`ffmpeg -f avfoundation -i ":1" -t 10 audiocapture.mp3`

Explanation
```
 -f avfoundation    use the MacOS native audio framework
 -i ":1"            use audio device 1
 -t 10              record for 10 seconds
 audiocapture.mp3   the file to save to
```

## Trying it out

1. I opened VCV Rack 2 Free and created a patch that generated sound
   continuously.
2. I set it's output device as my "Blackhole 2ch" audio virtual cable
3. I used `ffmpeg -f avfoundation -i ":1" -t 10 audiocapture.mp3` to record 10
   seconds of audio.
4. Only 7 seconds was recorded and using VLC to play the file showed that the
   audio quality was jittery and bad.

The Audio Plugin in VCV Rack 2 was set to 48khz and a block size of 256, saying
that this was a latency of 5.3mS. Setting the block size to 4096 did not seem to
help at all.

Setting the audio output to a real device (MacBook Air Speakers) had no audio
quality issue.

I don't know whether the poor audio quality was due to ffmpeg, Blackhole, or
VCV Rack 2, but I suspect it is either ffmpeg or Blackhole.

I tried recording to `audiocapture.wav` to use WAV format audio, whcih it did.
However the audio quality was still bad.

## sox
[The answer here](https://superuser.com/questions/1601224/ffmpeg-with-blackhole-audio-crackling-noises-why) and in many other places suggest that `ffmpeg`
simply does not work as it should. Instead, you should use `sox`.

I installed `sox` with `brew install sox`. It installed version 14.4.2.

1. I recorded `sox -t coreaudio "BlackHole 2ch" test.wav`.
2. I had to stop the recording manually with CTRL-C.
3. Audio quality was excellent.

I then tried recroding an mp3 stream with 
`sox -t coreaudio "BlackHole 2ch" test.mp3`. This also worked well.

The MP3 was 10 seconds long and 166KB. The WAV was 7s and 2.6MB.

Additionally, `sox` gave a live preview of the recording levels in the terminal
as it recorded.


