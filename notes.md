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



