# Dvorak-Lock: Providing a Dvorak Shift, Lock and Toggle feature for Linux distros

Being a Dvorak typist sucks if you also want to play games, because none of the operating systems provide a good mechanism out-of-the-box for handling QWERTY controls with Dvorak typing. On Windows this has been a solved problem for a long time as you can simply bodge a driver together by editing MSKLC source files (https://www.theleruby.com/keyboard/) which has worked for me without much issue since ~2012. Unfortunately on Linux things are still quite dire. Now that we have Proton to make Linux gaming more realistic, this desperately needed another look.

Thomas Bocek wrote a Wayland-compatible Dvorak<>QWERTY keyboard remapping service (https://github.com/tbocek/dvorak) which remaps to QWERTY while you hold a modifier key, and I was using that for a while on servers and in VMs and stuff. It's perfectly adequate if you only use Linux for development or terminal use and don't need to play games, but if you do need to play games it's not ideal (it requires the keyboard layout to be set to Dvorak, which causes problems with both Steam Input and the SteamOS virtual keyboard, and it's also not Wine/Proton-friendly).

This tool is a fork of that service which I've used to prototype a more practical solution that is better suited to gaming. It's in a crude state at the moment and needs a lot of refinement but it's usable if you need something urgent.

## How does this work?

This tool implements Dvorak Shift, Dvorak Lock and Dvorak Toggle Mode which are overlayed on top of a US or UK QWERTY layout.

The "Dvorak Lock" is toggled by pressing Scroll Lock. While enabled, the keyboard gets remapped to Dvorak. While disabled, the keyboard stays as QWERTY.

The "Dvorak Shift" feature is activated by holding Caps Lock down. This lock state will be inverted while the key is held. If you hold Caps Lock while in QWERTY mode, you get Dvorak. If you hold Caps Lock while in Dvorak mode, you get QWERTY.

The 'Caps Lock' indicator is used to indicate the state of the Dvorak Lock (it might have made more sense to use the Scroll Lock indicator, but neither my Logitech keyboard nor my gaming laptop have one - a lot of keyboards don't these days!)

If the LCTRL / RCTRL / LALT / LMETA / RMETA keys are held down before pressing a key, the remapping will not take place (the key will always be treated as QWERTY).

This remapping service has been tested with Fedora Kinoite 43 along with several variants of it. It should work on any modern Linux distro.

## Toggle Mode

Toggle Mode is an extra feature which can be enabled by passing `-t`. When this feature is enabled the behaviour is changed slightly:

* **Scroll Lock**: Dvorak layout ON/OFF
* **CTRL + Scroll Lock** Dvorak layout ON, Toggle Mode OFF
* **ALT + Scroll Lock**: Dvork layout OFF, Toggle Mode OFF
* **META + Scroll Lock**: Dvorak layout OFF, Toggle Mode ON

Toggle Mode is designed to make it easier to play video games like World of Warcraft where you have to type into the chat box a lot. While enabled, pressing ENTER behaves like you pressed Scroll Lock (Dvorak layout ON/OFF). Pressing ESC turns Dvorak layout OFF. The use case is essentially this: press QWERTY WASD to move around, press ENTER to open the chat box (which also activates Dvorak), then type your message in Dvorak, then press ENTER to send the message and close the chat box (which also deactivates Dvorak). For this to be useful, you must obviously have the 'open chat box' button in the game set to the ENTER key.

While in Toggle Mode, pressing Scroll Lock by itself will alternate the Dvorak ON/OFF state without deactivating Toggle Mode. This can be useful if e.g. you have to type your username into a login prompt. Key sequence behaviour is like this: At the login prompt, press Scroll Lock (Dvorak turns on), type your username in Dvorak, press TAB to switch to password box, type password in Dvorak, press ENTER (Dvorak turns off).

## How to run / install

You need to use sudo. The following parameters can be used:

```
usage: dvorak [OPTION]
  -d /dev/input/by-id/… Specifies which device should be captured.
  -m STRING             Match only the STRING with the USB device name. 
                        STRING can contain multiple words, separated by space.
  -l                    Toggle state of a dummy LED on every key press
                        (works around LED turning itself off on some keyboards)
  -t                    Enable toggle mode feature

example: sudo ./dvorak -u -d /dev/input/by-id/usb-Logitech_USB_Receiver-if02-event-kbd -m "k750 k350"
```

You use the `-m` argument to restrict the program so that it only runs for specific input devices. If the name of the device passed via `-d` argument doesn't contain one of the strings passed into the `-m` argument then that copy of the program exits.

Some keyboards turn the indicator LEDs off while the keyboard is idle. This causes the Caps Lock indicator to end up in the wrong state. If you have one of these keyboards (e.g. Logitech G915 X) you should pass the `-l` flag which makes the LED state get refreshed on every key press by toggling the compose LED (which you probably don't have, and thus won't notice being toggled).

Passing `-t` enables the toggle mode feature.

Installing as a service will make the program run automatically whenever an input device is attached to the system. 

 * create binary with ```make```
 * modify `dvorak@.service` file to add the required `-m` argument at the end
 * install it with ```sudo make install```

## Uninstall

```
sudo make uninstall
```

## Future Plans

This remapping service is a bit of a rushed bodge. I wanted to create something usable so that I could test out the viability of using Linux as a daily driver. In the event that I decide to adopt Linux longer-term, I'd like to extend the behaviour so that the state can be remembered on a per-window basis, and maybe add GUI features like a settings screen and a tray icon. I suspect that will end up being a partial or complete rewrite, so this project will probably not get many further updates.
