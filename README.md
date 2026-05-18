# Prusa-Bootloader-Puppy
This is a bootloader for various daughterboards of Prusa printers ("puppies")
comunicating over RS845, originally based on the [Childbus Bootloader](https://github.com/3devo/ChildbusBootloader).

The motherboard talks to its puppies over Puppybus (Modbus on RS485).
This bootloader is the first thing that starts on the puppies.
The bootloader does not start any application on startup by itself, nor it
initiates any communication over the Puppybus.

Instead, when the motherboard decides, it talks to the puppies running this bootloader and
uses the bootloader to flash the latest firmware, set the Puppybus address and
eventually, start the firmware itself on them.

## License
The bootloader is based on the [Childbus Bootloader](https://github.com/3devo/ChildbusBootloader)
from [3devo](https://github.com/3devo),
which is based on the bootloader written by Erin Tomson for the Modulo board.

Most code and accompanying materials are licensed under the GPL, version 3.
See the LICENSE file for the full license.

The protocol documentation, in the PROTOCOL.md file, has a more liberal
license (see the file for the exact license).
