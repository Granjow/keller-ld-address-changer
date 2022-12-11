# Keller LD Address Changer

This project allows to change the I²C address of a [Keller][keller] LD sensor
(4LD…9LD). These are I²C pressure sensors which we also read out with an
Arduino at [Geheimgang][g188].

The [BlueRobotics][br] library reads out the sensor values.

**Warning:** The I²C address is stored in a memory which can only write `1`s,
so **byte writes are irrevertible.**

**Warning:** This tool has not been tested.

[keller]: https://keller-druck.com/
[br]: https://github.com/bluerobotics/BlueRobotics_KellerLD_Library
[g188]: https://geheimgang.ch/
