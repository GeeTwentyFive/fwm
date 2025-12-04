#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <stdlib.h>


#define MAX_WINDOWS 64

#define MODIFIER Mod4Mask // Windows key
#define START_TERMINAL_KEY XK_space // Start terminal emulator
#define KILL_WINDOW_KEY XK_Escape // Kill focused window
#define MOVE_LEFT_KEY XK_Control_L // Decrement selected window index
#define MOVE_RIGHT_KEY XK_Alt_L // Increment selected window index


Window windows[MAX_WINDOWS];
int windows_count = 0;
int selected_window_index = 0;

XWindowChanges fullscreen_window_changes = {0};

KeyCode start_terminal_keycode;
KeyCode kill_window_keycode;
KeyCode move_left_keycode;
KeyCode move_right_keycode;

char* terminal_emulator;

int main(int argc, char* argv[]) {
        if (argc != 2) {
                puts("USAGE: fwm <TERMINAL_EMULATOR>");
                return 1;
        }

        terminal_emulator = argv[1];


        Display* display = XOpenDisplay(NULL);
        if (!display) {
                fprintf(stderr, "ERROR: Failed to connect to X server\n");
                return 1;
        }

        int screen = DefaultScreen(display);
        int screen_width = DisplayWidth(display, screen);
        int screen_height = DisplayHeight(display, screen);

        fullscreen_window_changes.width = screen_width;
        fullscreen_window_changes.height = screen_height;

        Window root = RootWindow(display, screen);

        XSelectInput(display, root, SubstructureRedirectMask|SubstructureNotifyMask);

        start_terminal_keycode = XKeysymToKeycode(display, START_TERMINAL_KEY);
        kill_window_keycode = XKeysymToKeycode(display, KILL_WINDOW_KEY);
        move_left_keycode = XKeysymToKeycode(display, MOVE_LEFT_KEY);
        move_right_keycode = XKeysymToKeycode(display, MOVE_RIGHT_KEY);

        XGrabKey(display, start_terminal_keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, kill_window_keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, move_left_keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, move_right_keycode, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);

        XSync(display, False);

        XEvent event;
        for (;;) {
                XNextEvent(display, &event);

                if (event.type == CreateNotify) { // Add to end of windows array (if not full) + fullscreen
                        if (windows_count == MAX_WINDOWS) {
                                XKillClient(display, event.xcreatewindow.window);
                                continue;
                        }

                        windows[windows_count] = event.xcreatewindow.window;
                        windows_count++;

                        XConfigureWindow(
                                display,
                                event.xcreatewindow.window,
                                CWX|CWY|CWWidth|CWHeight,
                                &fullscreen_window_changes
                        );
                }
                else if (event.type == DestroyNotify) { // Remove from windows array
                        int window_index = -1;
                        for (int i = 0; i < windows_count; i++) {
                                if (windows[i] == event.xdestroywindow.window) { window_index = i; break; }
                        }

                        if (window_index == -1) continue;

                        windows[window_index] = 0; // (if at end)
                        for (int i = window_index; i < windows_count-1; i++) {
                                windows[i] = windows[i+1];
                        }
                        windows_count--;
                        selected_window_index--;
                        if (selected_window_index == -1) selected_window_index = 0;
                }
                else if (event.type == KeyPress) {
                        if (!(event.xkey.state & MODIFIER)) continue;

                        KeyCode keycode = event.xkey.keycode;
                        if (keycode == start_terminal_keycode) {
                                system(terminal_emulator);
                        }
                        else if (keycode == kill_window_keycode) {
                                if (windows_count == 0) continue;

                                XKillClient(display, windows[selected_window_index]);
                        }
                        else if (keycode == move_left_keycode) {
                                if (windows_count == 0) continue;

                                if (selected_window_index == 0)
                                        selected_window_index = windows_count-1;
                                else
                                        selected_window_index--;

                                XRaiseWindow(display, windows[selected_window_index]);
                        }
                        else if (keycode == move_right_keycode) {
                                if (windows_count == 0) continue;

                                if (selected_window_index == windows_count-1)
                                        selected_window_index = 0;
                                else
                                        selected_window_index++;

                                XRaiseWindow(display, windows[selected_window_index]);
                        }
                }
        }

        XCloseDisplay(display);

        return 0;
}