//=================================================================================================
// Dvorak keyboard remapping service
//=================================================================================================
// This is a modified version of the Dvorak<>QWERTY keyboard remapping service written by Thomas Bocek (https://github.com/tbocek/dvorak).
//
// This implementation is backwards to the original. The keyboard layout should be left set to QWERTY and reports as QWERTY to all programs.
// The operating system must also be configured to disable CAPS LOCK, as this program replaces the CAPS LOCK behaviour.
//
// Pressing SCROLL LOCK toggles a 'Dvorak Lock' mode on/off. While the lock mode is enabled, the keyboard is remapped to Dvorak.
// Holding CAPS LOCK will make the keyboard behave as though the opposite lock state is being used.
// The LED indicator for CAPS LOCK is used to indicate the current lock state.
//
// When the LCTRL/RCTRL/LALT/LMETA/RMETA keys are held down before pressing a key the remapping will never take place (the key will always be treated as QWERTY).
//

/*
 * Copyright 2018 Thomas Bocek
 * Copyright 2026 Ruby Dennington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 */

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/uinput.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>

#define MAX_LENGTH 32

static int fdi;
static volatile sig_atomic_t keep_running = 1;
static void sig_handler(int sig) {
    keep_running = 0;
    close(fdi);
}

//from: https://github.com/kentonv/dvorak-qwerty/tree/master/unix
static int modifier_bit(int key) {
    switch (key) {
        case KEY_LEFTCTRL:
            return 1;
        case KEY_RIGHTCTRL:
            return 2;
        case KEY_LEFTALT:
            return 4;
        case KEY_LEFTMETA:
            return 8;
        case KEY_RIGHTMETA:
            return 16;
        default:
            return 0;
    }
}

static int qwerty2dvorak(int key) {
    switch (key) {
        // number row
        case KEY_MINUS:
            return KEY_LEFTBRACE;
        case KEY_EQUAL:
            return KEY_RIGHTBRACE;
        // top row
        case KEY_Q:
            return KEY_APOSTROPHE;
        case KEY_W:
            return KEY_COMMA;
        case KEY_E:
            return KEY_DOT;
        case KEY_R:
            return KEY_P;
        case KEY_T:
            return KEY_Y;
        case KEY_Y:
            return KEY_F;
        case KEY_U:
            return KEY_G;
        case KEY_I:
            return KEY_C;
        case KEY_O:
            return KEY_R;
        case KEY_P:
            return KEY_L;
        case KEY_LEFTBRACE:
            return KEY_SLASH;
        case KEY_RIGHTBRACE:
            return KEY_EQUAL;
        // middle row
        case KEY_A:
            return KEY_A;
        case KEY_S:
            return KEY_O;
        case KEY_D:
            return KEY_E;
        case KEY_F:
            return KEY_U;
        case KEY_G:
            return KEY_I;
        case KEY_H:
            return KEY_D;
        case KEY_J:
            return KEY_H;
        case KEY_K:
            return KEY_T;
        case KEY_L:
            return KEY_N;
        case KEY_SEMICOLON:
            return KEY_S;
        case KEY_APOSTROPHE:
            return KEY_MINUS;
        // bottom row
        case KEY_Z:
            return KEY_SEMICOLON;
        case KEY_X:
            return KEY_Q;
        case KEY_C:
            return KEY_J;
        case KEY_V:
            return KEY_K;
        case KEY_B:
            return KEY_X;
        case KEY_N:
            return KEY_B;
        case KEY_M:
            return KEY_M;
        case KEY_COMMA:
            return KEY_W;
        case KEY_DOT:
            return KEY_V;
        case KEY_SLASH:
            return KEY_Z;
        default:
            return key;
    }
}

static ssize_t emit(int fd, int type, int code, int value, struct timeval time) {
    struct input_event ev = {0};
    ev.type = type;
    ev.code = code;
    ev.value = value;
    ev.time = time;
    //fprintf(stdout, "Emit event type=%d code=%d value=%d\n",ev.type, ev.code, ev.value);
    return write(fd, &ev, sizeof(ev));
}

static bool has_event_type(const unsigned int array_bit_ev[], int event_type) {
    return (array_bit_ev[event_type/32] & (1U << (event_type % 32))) != 0;
}

static bool setup_event_type(int fdo, unsigned long event_type, int max_val, const unsigned int array_bit[]) {
    struct uinput_abs_setup abs_setup = {};
    bool abs_init_once = false;

    for (int i = 0; i < max_val; i++) {
        if (!(array_bit[i / 32] & (1U << (i % 32)))) {
            continue;
        }

        //fprintf(stderr, "Setting capability %d for event type %lu\n", i, event_type);
        switch(event_type) {
            case UI_SET_EVBIT:
                if (ioctl(fdo, UI_SET_EVBIT, i) < 0) {
                    fprintf(stderr, "Cannot set EV bit %d: %s\n", i, strerror(errno));
                    return false;
                }
                break;
            case UI_SET_KEYBIT:
                if (ioctl(fdo, UI_SET_KEYBIT, i) < 0) {
                    fprintf(stderr, "Cannot set KEY bit %d: %s\n", i, strerror(errno));
                    return false;
                }
                break;
            case UI_SET_RELBIT:
                if (ioctl(fdo, UI_SET_RELBIT, i) < 0) {
                    fprintf(stderr, "Cannot set REL bit %d: %s\n", i, strerror(errno));
                    return false;
                }
                break;
            case UI_SET_ABSBIT:
                if (!abs_init_once) {
                    abs_setup.code = i;
                    if (ioctl(fdi, EVIOCGABS(i), &abs_setup.absinfo) < 0) {
                        fprintf(stderr, "Failed to get ABS info for axis %d: %s\n", i, strerror(errno));
                        continue;
                    }
                    if (ioctl(fdo, UI_ABS_SETUP, &abs_setup) < 0) {
                        fprintf(stderr, "Failed to setup ABS axis %d: %s\n", i, strerror(errno));
                        continue;
                    }
                    abs_init_once = true;
                }

                if (ioctl(fdo, UI_SET_ABSBIT, i) < 0) {
                    fprintf(stderr, "Cannot set ABS bit %d: %s\n", i, strerror(errno));
                    return false;
                }
                break;
            case UI_SET_MSCBIT:
                if (ioctl(fdo, UI_SET_MSCBIT, i) < 0) {
                    fprintf(stderr, "Cannot set MSC bit %d: %s\n", i, strerror(errno));
                    return false;
                }
                break;
        }
    }
    return true;
}

static void usage(const char *path) {
    /* take only the last portion of the path */
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path;

    fprintf(stderr, "usage: %s [OPTION]\n", basename);
    fprintf(stderr, "  -d /dev/input/by-id/…\t"
                    "Specifies which device should be captured.\n");
    fprintf(stderr, "  -m STRING\t\t"
                    "Match only the STRING with the USB device name. \n"
                    "\t\t\tSTRING can contain multiple words, separated by space.\n");
    fprintf(stderr, "  -l\t\t\t"
                    "Toggle state of a dummy LED on every key press\n"
                    "\t\t\t(works around LED turning itself off on some keyboards)\n");
    fprintf(stderr, "  -t\t\t\t"
                    "Enable enter toggle mode feature\n");
    fprintf(stderr, "example: %s -u -d /dev/input/by-id/usb-Logitech_USB_Receiver-if02-event-kbd -m \"k750 k350\"\n", basename);
}

int main(int argc, char *argv[]) {
    signal(SIGTERM, sig_handler);

    int opt;
    char *device = NULL,
         *match = NULL;
    bool use_dummy_led = false;
    bool use_enter_toggle_mode_feature = false;
    while ((opt = getopt(argc, argv, "d:m:lt")) != -1) {
        switch (opt) {
            case 'd':
                device = optarg;
                break;
            case 'm':
                match = optarg;
                break;
            case 'l':
                use_dummy_led = true;
                break;
            case 't':
                use_enter_toggle_mode_feature = true;
                break;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (device == NULL) {
        usage(argv[0]);
        fprintf(stderr, "Error: Input device not specified.\n");
        fprintf(stderr, "Hint: Provide a valid input device, typically found under /dev/input/by-id/...\n");
        return EXIT_FAILURE;
    }

    //Start the fdi setup
    fdi = open(device, O_RDWR);
    if (fdi < 0) {
        fprintf(stderr, "Error: Failed to open device [%s]: %s.\n", device, strerror(errno));
        fprintf(stderr, "Hint: Check if the device path is correct and you have the necessary permissions.\n");
        return EXIT_FAILURE;
    }

    char keyboard_name[UINPUT_MAX_NAME_SIZE] = "Unknown";
    int ret_val = ioctl(fdi, EVIOCGNAME(sizeof(keyboard_name) - 1), keyboard_name);
    if (ret_val < 0) {
        fprintf(stderr, "Error: Unable to retrieve device name for [%s]: %s.\n", device, strerror(errno));
        fprintf(stderr, "Hint: Verify if the device is functional and properly configured.\n");
        close(fdi);
        return EXIT_FAILURE;
    }

    struct uinput_setup usetup =
            { .id =
                { .bustype = BUS_USB, .vendor = 0x1111, .product = 0x2222 },
              .name = "Virtual Dvorak Keyboard" };
    if (strcmp(keyboard_name, usetup.name) == 0) {
        fprintf(stdout, "Info: Skipping mapping for the device we just created: %s.\n", keyboard_name);
        close(fdi);
        return EXIT_SUCCESS;
    }

    ret_val = -1;
    if (match != NULL) {
        char *token = strtok(match, " ");
        while (token != NULL) {
            if (strcasestr(keyboard_name, token) != NULL) {
                printf("Info: Found matching input: [%s] for device [%s].\n", keyboard_name, device);
                ret_val = 0;
                break;
            }
            token = strtok(NULL, " ");
        }
        if (ret_val < 0) {
            fprintf(stderr, "Error: Device [%s] does not match any of the specified keywords: [%s].\n", keyboard_name, match);
            close(fdi);
            return EXIT_FAILURE;
        }
    }

    // Read capabilities
    unsigned int
        array_bit_ev[EV_MAX/32 + 1]= {0},
        array_bit_key[KEY_MAX/32 + 1]= {0},
        array_bit_rel[REL_MAX/32 + 1]= {0},
        array_bit_abs[ABS_MAX/32 + 1]= {0},
        array_bit_msc[MSC_MAX/32 + 1]= {0};

    ret_val = ioctl(fdi, EVIOCGBIT(0, sizeof(array_bit_ev)), &array_bit_ev);
    if (ret_val < 0) {
        fprintf(stderr, "Error: Failed to retrieve event capabilities for device [%s]: %s.\n", device, strerror(errno));
        close(fdi);
        return EXIT_FAILURE;
    }

    if (has_event_type(array_bit_ev, EV_KEY)) {
        ret_val = ioctl(fdi, EVIOCGBIT(EV_KEY, sizeof(array_bit_key)), &array_bit_key);
        if (ret_val < 0) {
            fprintf(stderr, "Error: Failed to retrieve EV_KEY capabilities for device [%s]: %s.\n", device, strerror(errno));
            close(fdi);
            return EXIT_FAILURE;
        }
    }

    if (has_event_type(array_bit_ev, EV_REL)) {
        ret_val = ioctl(fdi, EVIOCGBIT(EV_REL, sizeof(array_bit_rel)), &array_bit_rel);
        if (ret_val < 0) {
            fprintf(stderr, "Error: Failed to retrieve EV_REL capabilities for device [%s]: %s.\n", device, strerror(errno));
            close(fdi);
            return EXIT_FAILURE;
        }
    }

    if (has_event_type(array_bit_ev, EV_ABS)) {
        ret_val = ioctl(fdi, EVIOCGBIT(EV_ABS, sizeof(array_bit_abs)), &array_bit_abs);
        if (ret_val < 0) {
            fprintf(stderr, "Error: Failed to retrieve EV_ABS capabilities for device [%s]: %s.\n", device, strerror(errno));
            close(fdi);
            return EXIT_FAILURE;
        }
    }

    if (has_event_type(array_bit_ev, EV_MSC)) {
        ret_val = ioctl(fdi, EVIOCGBIT(EV_MSC, sizeof(array_bit_msc)), &array_bit_msc);
        if (ret_val < 0) {
            fprintf(stderr, "Error: Failed to retrieve EV_MSC capabilities for device [%s]: %s.\n", device, strerror(errno));
            close(fdi);
            return EXIT_FAILURE;
        }
    }

    //Check we are a keyboard
    if (!(array_bit_key[KEY_X / 32] & (1 << (KEY_X % 32))) ||
        !(array_bit_key[KEY_C / 32] & (1 << (KEY_C % 32))) ||
        !(array_bit_key[KEY_V / 32] & (1 << (KEY_V % 32)))) {
        fprintf(stdout, "Info: Device [%s] is not recognized as a keyboard (missing essential keys).\n", device);
        close(fdi);
        return EXIT_SUCCESS;
    }

    // Start the uinput setup
    int fdo = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fdo < 0) {
        fprintf(stderr, "Error: Failed to open /dev/uinput for device [%s]: %s.\n", device, strerror(errno));
        close(fdi);
        return EXIT_FAILURE;
    }

    // Configure the virtual device
    if (ioctl(fdo, UI_DEV_SETUP, &usetup) < 0) {
        fprintf(stderr, "Error: Failed to configure the virtual device for [%s]: %s.\n", device, strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    if(!setup_event_type(fdo, UI_SET_EVBIT, EV_SW, array_bit_ev)) {
        fprintf(stderr, "Cannot setup_event_type for UI_SET_EVBIT/device [%s]: %s.\n", device, strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    if(!setup_event_type(fdo, UI_SET_KEYBIT, KEY_MAX, array_bit_key)) {
        fprintf(stderr, "Cannot setup_event_type for EV_KEY/device [%s]: %s.\n", device, strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    if(!setup_event_type(fdo, UI_SET_RELBIT, REL_MAX, array_bit_rel)) {
        fprintf(stderr, "Cannot setup_event_type for EV_REL/device [%s]: %s.\n", device, strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    if(!setup_event_type(fdo, UI_SET_ABSBIT, ABS_MAX, array_bit_abs)) {
        fprintf(stderr, "Cannot setup_event_type for EV_ABS/device [%s]: %s.\n", device, strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    if(!setup_event_type(fdo, UI_SET_MSCBIT, MSC_MAX, array_bit_msc)) {
        fprintf(stderr, "Cannot setup_event_type for MSC_MAX/device [%s]: %s.\n", device, strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    if (ioctl(fdo, UI_DEV_CREATE) < 0) {
        fprintf(stderr, "Cannot create device: %s.\n", strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    // Wait for device to be ready
    usleep(200000);

    if (ioctl(fdi, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "Cannot grab key: %s.\n", strerror(errno));
        close(fdo);
        close(fdi);
        return EXIT_FAILURE;
    }

    struct input_event ev = {0};
    int mod_state = 0,
        array_dvorak_counter = 0;
    bool disable_mapping = false;
    unsigned int array_dvorak[MAX_LENGTH] = {0};
    bool is_dvoraking = true;
    bool is_capslock_held = false;
    bool is_enter_toggle_mode = false;
    bool dummy_led_flag = false;

    fprintf(stderr, "Starting event loop with keyboard: [%s] for device [%s].\n", keyboard_name, device);

    fprintf(stderr, "Setting initial keyboard LED state\n");
    emit(fdi, EV_LED, 1, is_dvoraking, ev.time);

    while (keep_running) {
        ssize_t n = read(fdi, &ev, sizeof ev);
        if (n == (ssize_t) -1) {
            if (errno == EINTR)
                continue;
            break;
        } else if (n != sizeof ev) {
            break;
        }

        int code_to_emit = ev.code;
        if (ev.type == EV_KEY && ev.code == KEY_SCROLLLOCK) {
            code_to_emit = KEY_KATAKANA;
        } else if (ev.type == EV_KEY && ev.code == KEY_CAPSLOCK) {
            code_to_emit = KEY_HIRAGANA;
        }

        if (ev.type == EV_KEY && ev.code == KEY_SCROLLLOCK && ev.value == 1) {
            if (use_enter_toggle_mode_feature) {
                if ((mod_state & 1) == 1 || (mod_state & 2) == 2) { // is CTRL held
                    // dvorak on
                    is_dvoraking = true;
                    is_enter_toggle_mode = false;
                } else if ((mod_state & 4) == 4) { // is left ALT held
                    // dvorak off
                    is_dvoraking = false;
                    is_enter_toggle_mode = false;
                } else if ((mod_state & 8) == 8 || (mod_state & 16) == 16) { // is Windows logo key held
                    // enter toggle mode
                    is_dvoraking = false;
                    is_enter_toggle_mode = true;
                /*
                } else if (is_enter_toggle_mode) {
                    // disable enter toggle mode, turn dvorak on
                    is_dvoraking = true;
                    is_enter_toggle_mode = false;
                */
                } else {
                    // invert current state
                    is_dvoraking = !is_dvoraking;
                    //is_enter_toggle_mode = false;
                }
            } else {
                is_dvoraking = !is_dvoraking;
                is_enter_toggle_mode = false;
            }
        }
        if (is_enter_toggle_mode && ev.type == EV_KEY && ev.value == 1) {
            // enter toggle mode. pressing enter toggles dvorak state. pressing esc cancels.
            if (ev.code == KEY_ENTER) {
                is_dvoraking = !is_dvoraking;
            } else if (ev.code == KEY_ESC) {
                is_dvoraking = false;
            }
        }
        if (use_dummy_led) {
            if ((ev.type == EV_KEY && ev.value == 1) || (ev.type == EV_LED && ev.code != 3)) {
                // if a key is pressed then we should update the LED
                emit(fdi, EV_LED, 1, is_dvoraking, ev.time);
                emit(fdi, EV_LED, 3, dummy_led_flag, ev.time); // alternate an unused LED as well to force refresh (specifically the sun microsystems compose LED). there might be a nicer way to do this, idk
                dummy_led_flag = !dummy_led_flag;
            }
        } else if ((ev.type == EV_KEY && ev.code == KEY_CAPSLOCK) || ev.type == EV_LED) {
            emit(fdi, EV_LED, 1, is_dvoraking, ev.time);
        }

        if(ev.type == EV_KEY) {
            if(ev.code == KEY_CAPSLOCK) {
                is_capslock_held = ev.value != 0;
            }

            int mod_current = modifier_bit(ev.code);

            if (mod_current > 0) {
                if (ev.value != 0) {
                    //set mod state when either 1 (key press), or 2 (repeat)
                    mod_state |= mod_current;
                } else {
                    //remove mod state when 0 (released)
                    mod_state &= ~mod_current;
                }
            }

            int dvorak_code = qwerty2dvorak(ev.code);
            if (ev.code != dvorak_code) {
                //pressed key
                if (ev.value == 1) {
                    //modifier not pressed
                    if(mod_state == 0 && (is_dvoraking != is_capslock_held) && !disable_mapping) {
                        if (array_dvorak_counter == MAX_LENGTH) {
                            printf("warning, too many keys pressed: %d. %s 0x%04x (%d), arr:%d\n",
                                MAX_LENGTH, ev.value == 1 ? "pressed" : "released", (int) ev.code, (int) ev.code,
                                array_dvorak_counter);
                        } else {
                            array_dvorak[array_dvorak_counter++] = dvorak_code;
                            //remap to dvorak - press key
                            emit(fdo, ev.type, dvorak_code, ev.value, ev.time);
                        }
                    } else {
                        //no modifier
                        emit(fdo, ev.type, code_to_emit, ev.value, ev.time);
                    }
                } else if(ev.value == 2) {
                    //repeating button
                    bool is_in_array = false;
                    for (int i = 0; i < array_dvorak_counter; i++) {
                        if (array_dvorak[i] == dvorak_code) {
                            is_in_array = true;
                            break;
                        }
                    }
                    if(is_in_array) {
                        //this is a repeating dvorak
                        emit(fdo, ev.type, dvorak_code, ev.value, ev.time);
                    } else {
                        //not in the array, regular key
                        emit(fdo, ev.type, code_to_emit, ev.value, ev.time);
                    }
                } else if(ev.value == 0) {
                    //release the key
                    bool need_emit = false;
                    for (int i = 0; i < array_dvorak_counter; i++) {
                        if (array_dvorak[i] == dvorak_code) {
                            array_dvorak[i] = 0;
                            need_emit = true;
                            break;
                        }
                    }
                    if(need_emit) {
                        int last_nonzero = -1;
                        for (int i = 0; i < array_dvorak_counter; i++) {
                            if (array_dvorak[i] != 0) {
                                last_nonzero = i;
                            }
                        }
                        array_dvorak_counter = last_nonzero + 1;
                        //remap to dvorak - release key
                        emit(fdo, ev.type, dvorak_code, ev.value, ev.time);
                    } else {
                        //regular qwerty key
                        emit(fdo, ev.type, code_to_emit, ev.value, ev.time);
                    }
                } else {
                    //this should not happen
                    emit(fdo, ev.type, code_to_emit, ev.value, ev.time);
                }
            } else {
                //regular qwerty key
                emit(fdo, ev.type, code_to_emit, ev.value, ev.time);
            }
        } else {
            //non regular key
            emit(fdo, ev.type, code_to_emit, ev.value, ev.time);
        }
    }
    close(fdi);
    close(fdo);
    return EXIT_SUCCESS;
}
