from Xlib import X
from Xlib.protocol import event


def send_fullscreen(conn, window, enabled):
    message = event.ClientMessage(window=window, client_type=conn.intern_atom("_NET_WM_STATE"),
                                  data=(32, [int(enabled), conn.intern_atom("_NET_WM_STATE_FULLSCREEN"), 0, 1, 0]))
    conn.screen().root.send_event(message, event_mask=X.SubstructureRedirectMask | X.SubstructureNotifyMask)
    conn.sync()
