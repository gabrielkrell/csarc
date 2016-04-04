# csarc
(cRIO) serial Arduino RGB Controller

Arduino program to control RGB lights over a serial connection.  Originally built for an off-season project (a t-shirt-shooting robot) on Northside's FRC Robotics team, and later used on our 2016 competition robot.

The goal of this project is a fairly robust serial connection that will allow the controlling device (cRIO, RoboRIO, etc.) to specify LED behavior directly or choose from built-in routines.  Use an RS232-TTL level converter to hook up the Arduino to the RIO.  Can be used with other serial sources (a laptop, another Arduino).

Unfortunately, Arduino doesn't like to use local libraries.  You'll have to install the two libraries used, either with Arduino's built-in library manager or manually.  If on Windows, with the default Arduino install location, you can just run install.bat.

Author Gabriel Krell, Jan 2016.
