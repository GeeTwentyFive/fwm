Fullscreen-only minimalistic X11 window manager

# Build
`gcc -O2 -march=native -mtune=native fwm -lX11 -o fwm`

# Install
1) Move `fwm` to `/usr/bin/`
2) `echo "exec fwm <YOUR_TERMINAL_EMULATOR>" > ~/.xinitrc`

# Usage
`startx`
- Win+Space - launch terminal
- Win+Escape - kill selected window
- Win+X - decrement selected window index
- Win+C - increment selected window index
