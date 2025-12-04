#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <stdio.h>


// TODO: Define windows doubly-linked list

int main() {
        Display* display = XOpenDisplay(NULL);
        if (!display) {
                fprintf(stderr, "ERROR: Failed to connect to X server\n");
                return 1;
        }

        int screen = DefaultScreen(display);
        int screen_width = DisplayWidth(display, screen);
        int screen_height = DisplayHeight(display, screen);

        Window root = RootWindow(display, screen);

        XSelectInput(display, root, SubstructureRedirectMask|SubstructureNotifyMask);

        XGrabKey(display, XKeysymToKeycode(display, XK_space), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, XKeysymToKeycode(display, XK_Escape), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, XKeysymToKeycode(display, XK_Left), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(display, XKeysymToKeycode(display, XK_Right), Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);

        // TODO: XQueryTree() -> add to windows list

        XSync(display, False);

        XEvent event;
        for (;;) {
                XNextEvent(display, &event);

                // TODO
        }

        XCloseDisplay(display);

        return 0;
}