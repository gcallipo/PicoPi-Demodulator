# PicoPi-Demodulator
This is a demodulator multimode AM SSB CW for quadrature input signal (es. taken from ElkSDR-UNO receiver or QRPLab Receiver o similar). The idea is to make a self contained with few controls "sistem on chip" demodulator. Using the ElkDSR-UNO and this project you will obtain a full working SDR receiver. The demodulator includes filtering, noise reduction and this new version 2.0.0 includes also a Panadapter with OLED display.

The project is work in progress.
vy 73 de Giuseppe, IK8YFW (@) libero.it

Video :

https://youtu.be/pxO1taTrk5k

https://youtube.com/shorts/FqbgBwjhAKQ

https://www.youtube.com/watch?v=xDynoE3Cdgc

https://www.youtube.com/watch?v=v5nKXXgiXFs

Video Panadapter:

https://www.youtube.com/watch?v=XJZHlI171lk

https://www.youtube.com/watch?v=LfmM74ldySU


Pico Pi demodulator (SSB CW AM ) and ElkSDR UNO Receiver as stand alone SDR Receiver.


HISTORY (recent upper):

[05.10.2024] - Create tag for versione 1.0.0 and Open a new versione 2.0.0
               First Commit of new version with Panadapter.
               Updated sources, main demodulator schematics and Video demostrations.

[02.05.2024] - Added the IowaHills program used to calculate the Hilbert Filters.

[13.08.2023] - Added new schematic. Apply a good I/Q audio level from an SDR receiver. For the ElkSDR-UNO you can increase the input audio level in using a small pre amplifier.


Credits: Thank you to JR3XNW for the code related the band scope waterfall.


