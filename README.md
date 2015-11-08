# Wiimedia Player

The Wiimedia Player is a Nintendo Wiimote controlled media player. This is an
application I built for a college class in 2009, however I am backing it up to
GitHub now for archival reasons. The rest of the readme is pulled directly from
the original project writeup.

## Functions

The first, and most important, aspects of a media player are the Pause and Play
buttons. When my application reads the signal for A or B from the Wiimote, then
the audio output will Play or Pause, respectively. Secondly, it is desirable to
be able to change songs, as well as fast forward and rewind. The Right and Left
buttons will control song changing, however I chose to utilize the
accelerometer for the X­axis to determine whether or not the Wiimote has been
rotated to the right or left. When the Home button is held and the Wiimore is
rotated to the right or left, the song will either fast forward or rewind.
Volume control is done through the + and – buttons on the Wiimote. Adding and
removing songs are done using the Up and Down buttons. When the Up button is
pressed, a random song from the library will be added to the current playlist,
however
￼￼
when the Down button is pressed, the currently playing song will be removed
from the playlist.

## Advanced Functions

Further utilization of the accelerometers is desired to selectively add songs
from the library. When the user holds button 1 and jerk the Wiimote to the
right or left, the library is scanned up or down the alphabet, respectively. To
choose specific artists, first the user must choose the desired letter of the
alphabet, and then hold button 2 and jerk right or left to scroll down or up
the artists that start with that letter. In order to add a song from that
artist, the user must hold B and then press the Up button.

## Project Specifications

The design and implementation of the Wiimedia Player was realized through the
help of two different APIs. First, the CWIID library, which allows bluetooth
connection to the Wiimote. Second, the libmpdclient library, which allows
control of the Music Player Daemon. The Wiimedia library works through the use
of constantly polling the current state from the Wiimote. Button pushes are
returned via two bit hex values, and g­ force information from the
accelerometers are returned as well. While more information is returned, it is
not used in my implementation.
