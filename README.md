# "Dvorak Lock" for Linux

This is a modified version of the Dvorak<>QWERTY keyboard remapping service written by Thomas Bocek (https://github.com/tbocek/dvorak). The primary intention is to implement both a "Dvorak Shift" and a "Dvorak Lock" which get overlayed on top of a US or UK QWERTY layout.

The "Dvorak Lock" is toggled by pressing Scroll Lock. While enabled, the keyboard gets remapped to Dvorak. While disabled, the keyboard stays as QWERTY.

The "Dvorak Shift" feature is activated by holding Caps Lock down. This lock state will be inverted while the key is held. If you hold Caps Lock while in QWERTY mode, you get Dvorak. If you hold Caps Lock while in Dvorak mode, you get QWERTY.

The 'Caps Lock' indicator is used to indicate the state of the Dvorak Lock (it might have made more sense to use the Scroll Lock indicator, but neither my Logitech keyboard nor my gaming laptop have one - a lot of keyboards don't these days!)

If the LCTRL / RCTRL / LALT / LMETA / RMETA keys are held down before pressing a key, the remapping will not take place (the key will always be treated as QWERTY).

This remapping service has been tested with Fedora Kinoite 43 along with several variants of it. It should work on any modern Linux distro.

## How to run / install

You need to use sudo. The following parameters can be used:

```
usage: dvorak [OPTION]
  -d /dev/input/by-id/… Specifies which device should be captured.
  -m STRING             Match only the STRING with the USB device name. 
                        STRING can contain multiple words, separated by space.
  -l                    Toggle state of a dummy LED on every key press
                        (works around LED turning itself off on some keyboards)

example: sudo ./dvorak -u -d /dev/input/by-id/usb-Logitech_USB_Receiver-if02-event-kbd -m "k750 k350"
```

You use the `-m` argument to restrict the program so that it only runs for specific input devices. If the name of the device passed via `-d` argument doesn't contain one of the strings passed into the `-m` argument then that copy of the program exits.

Some keyboards turn the indicator LEDs off while the keyboard is idle. This causes the Caps Lock indicator to end up in the wrong state. If you have one of these keyboards (e.g. Logitech G915 X) you should pass the `-l` flag which makes the LED state get refreshed on every key press by toggling the compose LED (which you probably don't have, and thus won't notice being toggled).

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
