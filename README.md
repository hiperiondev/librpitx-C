# librpitx-C
Translation to C of https://github.com/F5OEO/librpitx

### TEST:
```
(while true; do cat audio_raw/music48000.raw; done) | csdr convert_i16_f | csdr gain_ff 0.2 | csdr fmmod_fc | ./librpitx-C -i- -m IQFLOAT -f 147360
```
