# csarc
(cRIO) serial Arduino RGB Controller

Arduino program to control RGB lights over a serial connection.  Initially used by Axiom 4787 for our t-shirt robot's LEDs; may be reused on future comp robots.

The goal of this project is a fairly robust serial connection that will allow the cRIO to specify LED behavior directly or choose from built-in routines.  To implement, use a RS232-TTL level converter to hook up the Arduino to the cRIO.  Yes, the cRIO is old hardware.  Can be used with any other serial source (a laptop, a real robot controller, etc.).

Unfortunately, Arduino doesn't like to use local libraries.  You'll have to install the two libraries used, either with the handy built-in library manager, or manually.  If you're super-lazy and also on Windows, with the default Arduino install location, you can just run install.bat.

Author Gabriel Krell, Jan 2016.
