# arduNanoEQControl

Control of a equatorial mount using an <B>Arduino Nano</B> controller and two <B>DRV8825</B> drivers. It is a simple control, for up-down, left and right directions. 

The speed selector is done with two swithes in the next way;

- One swith to select over 3 speeds, x1, x8 and x16 times the sidereal rate.
- One "turbo" switch that multiplies the speed per 50 if it is on, so we can reach speeds of x50, x400 and x800
