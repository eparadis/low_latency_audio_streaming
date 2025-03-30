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

### Trying it out

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

### Trying it out

1. I recorded `sox -t coreaudio "BlackHole 2ch" test.wav`.
2. I had to stop the recording manually with CTRL-C.
3. Audio quality was excellent.

I then tried recroding an mp3 stream with 
`sox -t coreaudio "BlackHole 2ch" test.mp3`. This also worked well.

The MP3 was 10 seconds long and 166KB. The WAV was 7s and 2.6MB.

Additionally, `sox` gave a live preview of the recording levels in the terminal
as it recorded.

`sox test.wav -d` played the earlier recorded earlier.

## netcat (`nc`)

`netcat` was already installed with MacOS, but I have it better in general to
install versions from Homebrew because it is easier to know what version you are
getting. Also they tend to be much newer and contain useful patches.

I installed `netcat` with `brew install netcat`. It installed version 0.7.1.

### Trying it out

The goal is to do something like this:
`sox test.wav - | nc 127.0.0.1 12345` in one window and 
`nc -l 12345 | sox - -d` in another
to send the previously recorded audio over `netcat` and play it out the default
audio device. And `nc` is fine with this, but `sox` wants more information about
the audio format being used.

The `sox` manual states there are four pieces of info involved:
- sample rate   (-r rate)
- sample size   (-b bits)
- data encoding (-e enc)
- channels      (-c channels)

`nc -u 127.0.0.1 12345 | play --buffer 32 -tcvsd -r8k -` and
`rec --buffer 32 -tcvsd -r8k - | nc -u -l 12345` 
almost seemed to work.

It felt like `nc` wasn't working right, so I went back to a simpler step and
tested just the UDP part.
`nc -u -l -p 5555`    You must run this first (the listener)
`nc -u localhost 5555`

If either of the two closes (ie: via CTRL-C), the other doesn't know about it
until it tries to do something.

Also note that if the listener is listening, and the sender connects, then
disconnects, the sender cannot re-connect. The listener looks to be working but
the second instance of the sender will fail.

The following actually worked:
1. start the listener with `nc -u -l -p 5555 | play --buffer 32 -tcvsd -r8k -`
2. start the sender with `rec --buffer 32 -tcvsd -r8k - | nc -u localhost 5555`
Since I was playing and recording on the same machine, there was an echo effect
as it recorded it's own playback. The period of this sounded like about 100 to
200mS. That isn't a good sign for latency.

### Connecting to an audio source

I want to record from the virtual audio cable to avoid having to deal with
switching input and whatnot with the script.

I found this example of doing a FX patch between CoreAudio devices:
`sox -V6 -r 44100 -t coreaudio "BlackHole 2ch" -t coreaudio "MacBook Air Speakers" reverb 100 50 100 100 100 3`

Since I was already playing "into" the Blackhole pipe at 48k, it would not open
at 44.1k and instead switched to 48k. I think this is a property of Blackhole,
not of `sox`. Further, with the `-V6` verbosity, I could see that when I played
into the BlackHole input at only 8k, it would complain about not being able to
set the MacBook's speakers to 8k and would switch back to 48k. The high level of
verbosity provided helpful info about the audio input and output formats too.

Also messing with the reverb was fun.

I was able to send audio through `nc` like this:
`nc -u -l -p 5555 | play --buffer 32 -t raw -b 16 -r8k -esigned-integer -c 1  -`
`sox -V6 -t coreaudio "BlackHole 2ch" -t raw -b 16 -r8k -esigned-integer -c 1 - | nc -u localhost 5555`

The first command is the listener. `nc` is listening on port 5555 for UDP data.

The second command is the sender. `sox` is opening the coreaudio device and then
playing it to stdout with a "raw" format of 16bit 8kHz signed-integer single-
channel audio.

The listener must have the same format for its "raw" input!

Another `--buffer` parameter for the sender should help reduce latency. There
seemed to be a noticable amount.

# Testing across an actual network connection

I logged into my Intel Mac Mini, which is also connected via gigabit ethernet to
the same switch as the M2 MacBook Air I had been using for the above work. I
used Homebrew to install `sox` and `netcat` with `brew install sox netcat`.

Since it has built-in speakers, I wanted to play the audio from the Mac MacMini.

This worked:
Listener, on the Mac Mini
`nc --verbose -l -p 5555 | play --buffer 32 -t raw -b 16 -r8k -esigned-integer -c 1 -`

Sender, on the MacBook Air
`sox -V6 --buffer 32  -t coreaudio "BlackHole 2ch" -t raw -b 16 -r8k -esigned-integer -c 1 - | nc --verbose 192.168.0.61  5555`

Note that it isn't using UDP. Adding the `-u` flag to both did work as well.
The sender reports a many buffer overruns with discarded data. And you can tell
in the audio there are glitches. The latency is noticable.

Counter intuitively, raising the sample rate to 48k seemed to greatly improve
the latency. It sounded nearly simultaneous. There were still many buffer errors
reported on the sender console.

Sender:
`sox -V6 --buffer 32  -t coreaudio "BlackHole 2ch" -t raw -b 16 -r48k -esigned-integer -c 1 - | nc --verbose -u 192.168.0.61  5555`

Reciever:
`nc --verbose -u -l -p 5555 | play --buffer 32 -t raw -b 16 -r48k -esigned-integer -c 1 -`

# Debugging

`play -t coreaudio "asdf" -n -stat` show stats while recording from a device

To use UDP, I had to specify the source address:
`nc --verbose -s 192.168.0.140  -u -l -p 31215 | sox -V6 -t raw -b 32 -r48k -e signed-integer -c 1 - -r48k  -d remix 1 1`

I also had to add a rule for port forwarding in my router, which isn't surprising.

# results

With UDP over the internet from Vermont to Washinton, we were able to run for 6m 13s (17.9MB) before the
connection dropped.

When both sides were on ethernet, audio quality for `-b32 r48k -c1` with UDP was very good. That is a high rate
audio setting in any case, but importantly there wasn't any stutter or drop outs.

Listening command:
`nc --verbose -s 192.168.0.140 -u -l -p 31215 | sox -V6 -t raw -b 32 -r48k -e signed-integer -c 1 - -r48k  -d remix 1 1`

Sending command:
`sox -V4 --type coreaudio "Clarett+ 8Pre" --type raw - remix -m 1-18 | nc --verbose -s 192.168.40.181 -u ${EXT_IP_A} ${EXT_PORT_A}`


We were almost exactly one beat round trip at 130.02bpm
130.02 beats  1 minute   1 sec    -> 0.002167 beats    ->    461.5 mS   
1 minute      60 sec    1000 mS        milliseconds          1 beat


https://www.aeronetworks.ca/2015/09/audio-networking-with-sox-and-netcat.html

# using socat

sender command:
`sox -V6 --type coreaudio "Clarett+ 8Pre" --type raw - remix -m 1-18 | socat -d -d -u STDIN UDP:192.168.40.69:5555,sourceport=5555`


receiver command:
`socat -d -d -u UDP-LISTEN:5555,bind=192.168.40.69,reuseaddr,sourceport=5555 STDOUT \
    | sox -V6 -t raw -b 32 -r48k -e signed-integer -c 1 - -r48k -d remix 1 1`




# localhost with ncat
`sox --buffer 32 -V6  --type coreaudio "BlackHole 2ch"  -traw -b16 -r16k -esigned-integer -c1 - | ncat localhost`
`ncat -l -i1 -k | sox -V6 -traw -b16 -r16k -esigned-integer -c1 - -d remix 1 1`


strangely, we seemed to see better latency performance with 48k over lower bit-rates. 96k was good too.

`sox --multi-threaded  --buffer 1024  -V6 --type coreaudio "BlackHole 2ch" -traw -b8 -r96k -esigned-integer -c1 - remix 1-2 | socat -d -d -u STDIN UDP:${EXT_IP_B}:${EXT_IP_B},sourceport=5555`

# Using the timing program

I had a robot write most of a console app to measure timing. The program has
bugs but basic usage goes something like the following. Note: this isn't
actually measuring the full loop back yet.

on the remote machine (acting as netcat listener):
`nc --verbose -l -p 5555 | ./console_loopback_timer -c 20 -v`

on the local machine (acting as netcat sender):
`cat /dev/zero |  nc --verbose 192.168.0.61  5555`

I added the ability to ignore lines at startup and it just generally tries to do
the most useful thing if it might not be exactly perfectly accurate.

You can use `-c` with the `homebrew` version of `GNU netcat` to close the socket
on the sender when EOF is reached (even though this isn't mentioned in the man
page at all...)

on the sender
`yes 'some example text that you could make longer if you want' | head -n 1000 |nc -c --verbose 192.168.0.129 5555`

on the receiver
`nc --verbose -l -p 5555 | ./console_loopback_timer -q`


