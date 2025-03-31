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

# Using the rate limited data generator program

`./rate_limited_data_generator -r 10 -b '65 90'` will generate the numbers 65 to
90 and back again forever.

## loop back test at 100 Hz

First, set up the receiver
`nc --verbose -l -u -p 5555 | ./console_loopback_timer/console_loopback_timer -c 100 -q`

Then start the sender in another terminal:
`./rate_limited_data_generator/rate_limited_data_generator -r 100 -b '0 100' | nc --verbose -u localhost 5555`

What's happenign is that the receiver is listening on port 5555 UDP. It's
listening for 100 lines. The sender is generating data at 100 Hz. The data is
just ASCII strings of numbers from 0 to 100. That is being sent to localhost
port 5555 over UDP.

After the receiver gets 100 lines, it closes the connection, which shuts down
the sender as well.

As an example, this is the result I got once:
```
Summary:
  Total lines received: 100
  Total time: 1198.166 ms
  Min/Avg/Max time: 1.066/11.982/12.638 ms
```

That's saying that on average, it took 11.982 ms between lines. We'd expect 100
Hertz to have 10 ms between lines. So that's just about 2 ms of 'latency' due
to who knows what.

It's also worth pointing out that at least one packet arrived early or something
because there's a minium value of only 1.066 ms. Nothing arrived too late,
however. The longest delay was 2.638 ms.

## setting up a reflector and doing round trip

On the remote box, which is the "reflector", I used:
```
mkfifo /tmp/fifooo
cat /tmp/fifooo | nc -vvv -u -l -p 5555 -s 192.168.0.61 > /tmp/fifooo
```

This makes a FIFO to bounce data through. For UDP you will probably also have to
use the `-s` flag to get it to work. Also note that this is the 'listener' in
terms of `netcat`.

On the local box, you will send data and measure what come back with:
`./rate_limited_data_generator/rate_limited_data_generator -r 8000 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 8000 -q`

As an example, over WiFi, I got:
```
Summary:
  Total lines received: 8000
  Total time: 1213.977 ms
  Min/Avg/Max time: 0.000/0.152/5.852 ms
```

Conceptually, if it took 1213ms to receive what should have been 1000ms worth of
data, it means some packets were lost along the way. And that some took 5.9ms to
arrive and they should have been more like 0.125us, shows that there were gaps
or something in the data.

For comparison, just running through a pipe locally produces:
```
./rate_limited_data_generator/rate_limited_data_generator -r 8000 -b '0 100' | ./console_loopback_timer/console_loopback_timer -c 8000 -q

Summary:
  Total lines received: 8000
  Total time: 1286.082 ms
  Min/Avg/Max time: 0.127/0.161/0.204 ms
```

(I ran this a few times and the results were not identical but they were pretty
similar.)

Notable differences is that the minimum time is very close to the rate limit of
0.125ms and the maximum time isn't too much beyond the rate limit. The average
is still ~40us more than the desired rate. It seems reasonable to consider this
the overhead since we're waiting for 8000 samples exactly and are using a loss-
less channel.

Also interesting is the similarities to going out and back over the WiFi. Both
took about 1260ms for one second's worth of data. I don't think I can account
for that yet.

I don't know what part of this method isn't capturing what I want to measure.
I don't think that the average time between samples results in a frequency of
6589 Hz in one case and 6211 Hz in the other is actually a problem. This is
sending single data points at a time, which is highly inefficient. In reality,
we would buffer some fixed amount of data points and send them all at once. For
example, simply taking buffering on 8 data points would result in a rate of 1000
Hertz. Simluating that across the network:
```
$ ./rate_limited_data_generator/rate_limited_data_generator -r 1000 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 1000 -q

Summary:
  Total lines received: 1000
  Total time: 1239.831 ms
  Min/Avg/Max time: 0.000/1.240/3.628 ms
```

It seems notable that the total time is the same. I wonder if this holds for
lower rates too.
```
$ ./rate_limited_data_generator/rate_limited_data_generator -r 500 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 500 -q

Summary:
  Total lines received: 500
  Total time: 1160.478 ms
  Min/Avg/Max time: 0.000/2.321/8.683 ms
```

```
$ ./rate_limited_data_generator/rate_limited_data_generator -r 100 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 100 -q

Summary:
  Total lines received: 100
  Total time: 1184.489 ms
  Min/Avg/Max time: 0.000/11.845/15.388 ms
```

```
$ ./rate_limited_data_generator/rate_limited_data_generator -r 50 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 50 -q

Summary:
  Total lines received: 50
  Total time: 1173.950 ms
  Min/Avg/Max time: 7.903/23.479/28.791 ms
```

Maybe there's some sampling problems if we go much lower, so lets try higher.
```
./rate_limited_data_generator/rate_limited_data_generator -r 5000 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 5000 -q

Summary:
  Total lines received: 5000
  Total time: 1205.077 ms
  Min/Avg/Max time: 0.000/0.241/5.949 ms
```

```
./rate_limited_data_generator/rate_limited_data_generator -r 11025 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 11025 -q

Summary:
  Total lines received: 11025
  Total time: 1254.189 ms
  Min/Avg/Max time: 0.000/0.114/7.254 ms
```

```
./rate_limited_data_generator/rate_limited_data_generator -r 22050 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 22050 -q

Summary:
  Total lines received: 22050
  Total time: 1260.649 ms
  Min/Avg/Max time: 0.000/0.057/5.465 ms
```

And notably, full CD audio rate just doesn't work
```
./rate_limited_data_generator/rate_limited_data_generator -r 44100 -b '0 100' |  nc --verbose -u 192.168.0.61 5555 | ./console_loopback_timer/console_loopback_timer -c 44100 -q
Warning: Overrun detected. Adjusting sleep time.
Warning: Overrun detected. Adjusting sleep time.
write(net): No buffer space available

Summary:
  Total lines received: 4254
  Total time: 248.333 ms
  Min/Avg/Max time: 0.000/0.058/9.487 ms
```


So the results of all these tests summarized in a table shows that the overhead
doesn't _really_ change between sample rates.
```
Rate   | Total time | Ideal sample period | Max time
-------+------------+---------------------+-----------
22050  | 1261       |    45.4 us          |  5.465 ms
11025  | 1254       |    90.7 us          |  7.254 ms
 8000  | 1214       |   125   us          |  5.852 ms
 5000  | 1205       |   200   us          |  5.949 ms
 1000  | 1240       |  1.0    ms          |  3.628 ms
  500  | 1160       |  2.0    ms          |  8.683 ms
  100  | 1184       | 10.0    ms          | 15.388 ms
   50  | 1173       | 20.0    ms          | 28.791 ms
```
So if we're okay with 20ms of "artificial" delay, we need to cram all the data
we want into a packet and send it every 50ms. In our test above it took up to
28.8ms between packets, which means 8.8ms more delay.

So probably the "rate" we should concern ourselves with is the rate at which the
data point bundles are sent. That perhaps could be a tunable parameter or even
something that the system measures and adapts over time. That is to say for any
given network distance and condition, there is probably a "sweet spot" where
the bundle transmit rate is high but the max time a packet takes is low. I added
a column in the table that suggests the sweet spot is around 1000 Hz. If the
audio was sampled at 48kHz, each packet would only need 48 samples. At 32bits
per sample, that's 1536 bytes. That's _suspiciously_ close to the standard MTU
of ethernet (1500). I don't know how the overhead works, but it stands to reason
that there would be a optimal point somewhere around there.

The next step is to build a measurement tool that uses the reflect technique
shown above to directly measure the time it takes different sized packets at
different rates. This is sounding a lot like an ICMP ping tool that leaves its
connections open.


