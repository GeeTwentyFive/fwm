#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>


#define MAX_WINDOWS 64


Window windows[MAX_WINDOWS];
int windows_count = 0;
int selected_window_index = 0;

XWindowChanges fullscreen_window_changes = {0};

int main() {
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

        XGrabKey(display, XKeysymToKeycode(display, XK_space), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, XKeysymToKeycode(display, XK_Escape), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, XKeysymToKeycode(display, XK_Left), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, XKeysymToKeycode(display, XK_Right), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);

        // Query for already-opened windows -> add to array
        Window *qroot, *qparent, **qchildren;
        int qchildren_count;
        if (XQueryTree(display, root, &qroot, &qparent, &qchildren, &qchildren_count)) {
                for (int i = 0; i < qchildren_count; i++) {
                        if (windows_count == MAX_WINDOWS) {
                                XKillClient(display, qchildren[i]);
                                continue;
                        }

                        windows[windows_count] = qchildren[i];
                        windows_count++;

                        XConfigureWindow(
                                display,
                                qchildren[i],
                                CWX|CWY|CWWidth|CWHeight,
                                &fullscreen_window_changes
                        );
                }
        }

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
                }
                else if (event.type == KeyPress) {
                        // TODO:
                        // For Win+Escape: kill selected_window's process, let DestroyNotify handler handle rest
                        // For Win+Space: start default terminal with qexec input -> exit command chain
                        // For Win+Right: selected_window = selected_window->next; + XRaiseWindow()
                        // For Win+Left: selected_window = selected_window->prev; + XRaiseWindow()
                }
        }

        XCloseDisplay(display);

        return 0;
}