# librpitx-C
Translation to C of https://github.com/F5OEO/librpitx

*COMPILE TEST:*
- git clone https://github.com/hiperiondev/librpitx-C.git
- cd librpitx-C/Release
- make
- cd ..

*TEST:*
```
(while true; do cat audio_raw/music48000.raw; done) | csdr convert_i16_f | csdr gain_ff 0.2 | csdr fmmod_fc | Release/librpitx-C -i- -m IQFLOAT -f 147360

(while true; do cat audio_raw/speech48000.raw; done) | csdr convert_i16_f | csdr gain_ff 6000 | csdr convert_f_samplerf 20833 |sudo Release/librpitx-C -i- -m RF -f 147360
```
