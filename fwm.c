#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>
#include <unistd.h>


#define MAX_WINDOWS 64

#define MODIFIER Mod4Mask // Windows key
#define START_TERMINAL_KEY XK_space // Start terminal emulator
#define KILL_WINDOW_KEY XK_Escape // Kill focused window
#define MOVE_LEFT_KEY XK_X // Decrement selected window index
#define MOVE_RIGHT_KEY XK_C // Increment selected window index

const int EXTRA_MODIFIERS[] = {
        0,
        LockMask,
        Mod2Mask,
        LockMask | Mod2Mask
};


Window windows[MAX_WINDOWS];
int windows_count = 0;
int selected_window_index = 0;

XWindowChanges fullscreen_window_changes = {0};

KeyCode start_terminal_keycode;
KeyCode kill_window_keycode;
KeyCode move_left_keycode;
KeyCode move_right_keycode;

char* terminal_emulator;

int (*default_x_error_handler)(Display*, XErrorEvent*);
int x_error_handler(Display* display, XErrorEvent* error_event) {
        if (error_event->error_code == BadWindow) return 0;

        return default_x_error_handler(display, error_event);
}
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

        default_x_error_handler = XSetErrorHandler(x_error_handler);

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

        for (int i = 0; i < sizeof(EXTRA_MODIFIERS)/sizeof(EXTRA_MODIFIERS[0]); i++) {
                XGrabKey(display, start_terminal_keycode, MODIFIER | EXTRA_MODIFIERS[i], root, True, GrabModeAsync, GrabModeAsync);
                XGrabKey(display, kill_window_keycode, MODIFIER | EXTRA_MODIFIERS[i], root, True, GrabModeAsync, GrabModeAsync);
                XGrabKey(display, move_left_keycode, MODIFIER | EXTRA_MODIFIERS[i], root, True, GrabModeAsync, GrabModeAsync);
                XGrabKey(display, move_right_keycode, MODIFIER | EXTRA_MODIFIERS[i], root, True, GrabModeAsync, GrabModeAsync);
        }

        XSync(display, False);

        XEvent event;
        for (;;) {
                XNextEvent(display, &event);

                if (event.type == MapRequest) { // Add to end of windows array (if not full) + fullscreen + focus
                        if (windows_count == MAX_WINDOWS) {
                                XKillClient(display, event.xmaprequest.window);
                                continue;
                        }

                        windows[windows_count] = event.xmaprequest.window;
                        selected_window_index = windows_count;
                        windows_count++;

                        XConfigureWindow(
                                display,
                                event.xmaprequest.window,
                                CWX|CWY|CWWidth|CWHeight,
                                &fullscreen_window_changes
                        );

                        XMapWindow(display, event.xmaprequest.window);

                        XRaiseWindow(display, event.xmaprequest.window);

                        XSetInputFocus(display, event.xmaprequest.window, RevertToParent, CurrentTime);
                }
                else if (event.type == DestroyNotify || event.type == UnmapNotify) { // Remove from windows array
                        int window_index = -1;
                        for (int i = 0; i < windows_count; i++) {
                                if (windows[i] == ((event.type == DestroyNotify) ? event.xdestroywindow.window : event.xunmap.window)) { window_index = i; break; }
                        }

                        if (window_index == -1) continue;

                        for (int i = window_index; i < windows_count-1; i++) {
                                windows[i] = windows[i+1];
                        }
                        windows_count--;
                        selected_window_index--;
                        if (selected_window_index == -1) selected_window_index = 0;

                        if (windows_count == 0) continue;

                        XRaiseWindow(display, windows[selected_window_index]);

                        XSetInputFocus(display, windows[selected_window_index], RevertToParent, CurrentTime);
                }
                else if (event.type == KeyPress) {
                        if (event.xkey.keycode == start_terminal_keycode) {
                                if (fork() == 0) {
                                        execvp(terminal_emulator, NULL);
                                }
                        }
                        else if (event.xkey.keycode == kill_window_keycode) {
                                if (windows_count == 0) continue;

                                XKillClient(display, windows[selected_window_index]);
                        }
                        else if (event.xkey.keycode == move_left_keycode) {
                                if (windows_count <= 1) continue;

                                if (selected_window_index == 0)
                                        selected_window_index = windows_count-1;
                                else
                                        selected_window_index--;

                                XRaiseWindow(display, windows[selected_window_index]);

                                XSetInputFocus(display, windows[selected_window_index], RevertToParent, CurrentTime);
                        }
                        else if (event.xkey.keycode == move_right_keycode) {
                                if (windows_count <= 1) continue;

                                if (selected_window_index == windows_count-1)
                                        selected_window_index = 0;
                                else
                                        selected_window_index++;

                                XRaiseWindow(display, windows[selected_window_index]);

                                XSetInputFocus(display, windows[selected_window_index], RevertToParent, CurrentTime);
                        }
                }
                else if (event.type == ConfigureRequest) { // Required; ignore
                        XWindowChanges wc;
                        wc.sibling = event.xconfigurerequest.above;
                        wc.stack_mode = event.xconfigurerequest.detail;
                        XConfigureWindow(
                                display,
                                event.xconfigurerequest.window,
                                event.xconfigurerequest.value_mask & (CWSibling | CWStackMode),
                                &wc
                        );
                }
        }

        XCloseDisplay(display);

        return 0;
}