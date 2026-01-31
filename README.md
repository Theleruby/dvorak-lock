# "Dvorak Lock" for Linux

This is a modified version of the Dvorak<>QWERTY keyboard remapping service written by Thomas Bocek (https://github.com/tbocek/dvorak). The primary intention is to replace Caps Lock with a "Dvorak Lock" so that Dvorak layout can easily be toggled on/off by pressing the Caps Lock key, and the Caps Lock indicator will light up appropriately to indicate that the Dvorak Lock is turned on.

If the LCTRL / RCTRL / LALT / LMETA keys are held down before pressing a key, the remapping will not take place (the key will be treated as QWERTY).

This remapping service has been tested with Fedora Kinoite 43 and probably works on any Linux distro.

## How to run / install

You need to use sudo. The following parameters can be used:

```
usage: dvorak [OPTION]
  -d /dev/input/by-id/… Specifies which device should be captured.
  -m STRING             Match only the STRING with the USB device name. 
                        STRING can contain multiple words, separated by space.

example: sudo ./dvorak -u -d /dev/input/by-id/usb-Logitech_USB_Receiver-if02-event-kbd -m "k750 k350"
```

You use the `-m` argument to restrict the program so that it only runs for specific input devices. If the name of the device passed via `-d` argument doesn't contain one of the strings passed into the `-m` argument then that copy of the program exits.

Installing as a service will make the program run automatically whenever an input device is attached to the system. 

 * create binary with ```make```
 * modify `dvorak@.service` file to add the required `-m` argument at the end
 * install it with ```sudo make install```

## Additional requirements
You need to disable the Caps Lock key so that it does nothing. In KDE Plasma this can be found here:
System Settings > Keyboard > Key Bindings > Caps Lock behaviour > Caps Lock is disabled

## Uninstall

```
sudo make uninstall
```
