#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "xwindow.h"

static Display *dpy;
static int scr;
static Window w;
static Atom wmDeleteWindow;

void create_window(int width, int height)
{
    dpy = XOpenDisplay(NULL);
    scr = DefaultScreen(dpy);
    w = XCreateSimpleWindow(
            dpy, 
            RootWindow(dpy, scr), 
            0, 0, width, height, 1,
            BlackPixel(dpy, scr), 
            WhitePixel(dpy, scr));
    wmDeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    XSelectInput(dpy, w, ClientMessage);
    XSetWMProtocols(dpy, w, &wmDeleteWindow, 1);
    XMapWindow(dpy, w);
}

int update_window(unsigned char *img_data, int width, int height)
{

    draw_img(img_data, width, height);
    return x_button_pressed();

}

void draw_img(unsigned char *data, int width, int height)
{
    XImage *img = XCreateImage(dpy, DefaultVisual(dpy, scr), 24, ZPixmap, 0, (char*) data, width, height, 32, 0);
    XPutImage(dpy, w, DefaultGC(dpy, DefaultScreen(dpy)), img, 0, 0, 0, 0, width, height);

    XDestroyImage(img);
}

int x_button_pressed()
{
    if (XPending(dpy))
    {
        XEvent e;
        XNextEvent(dpy, &e);
        switch (e.type)
        {
            case ClientMessage:
                if (e.xclient.data.l[0] == wmDeleteWindow)
                {
                    close_window();
                    return 1;
                }
                break;
        }
    }
    return 0;
}

void close_window()
{
    XDestroyWindow(dpy, w);
    XCloseDisplay(dpy);
}
