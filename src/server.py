#!/usr/bin/env python3

import dbus
import dbus.mainloop.glib
from http.server import HTTPServer, BaseHTTPRequestHandler
import socket
from gi.repository import GLib


dbus_response = None

loop = GLib.MainLoop()
    
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()


def handle_dbus_response(response, results, object_path):
    global dbus_response
    dbus_response = results
    loop.quit()


def bus_call_picker(
        open_ = False, 
        save_ = False,
        multiple_ = False,
        directory_ = False,
        proposed_name_ = ""
    ):
    global bus
    global loop 
    global dbus_response

    obj = bus.get_object('org.freedesktop.portal.Desktop', '/org/freedesktop/portal/desktop')
    inter = dbus.Interface(obj, 'org.freedesktop.portal.FileChooser')

    if open_:
        inter.OpenFile(
            '', 
            'Open File' if not multiple_ else 'Select files', 
            { 
                'multiple': multiple_,
                'directory': directory_
            }
        )
    elif save_:
        inter.SaveFile('', 'Save File', {
            'current_name': proposed_name_
        })
    else:
        print('wtf do you want???')
        return None

    bus.add_signal_receiver(
        handle_dbus_response,
        signal_name='Response',
        dbus_interface='org.freedesktop.portal.Request',
        bus_name='org.freedesktop.portal.Desktop',
        path_keyword='object_path'
    )

    loop.run()

    return dbus_response


def bus_open_file_manager(location):
    global bus
    global loop
    global dbus_response

    obj = bus.get_object('org.freedesktop.FileManager1', '/org/freedesktop/FileManager1')
    inter = dbus.Interface(obj, 'org.freedesktop.FileManager1')

    return inter.ShowFolders([location], '')



class S(BaseHTTPRequestHandler):
    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
    

    def _opensave(self):
        dbus_result = bus_call_picker(
            open_=(self.path == '/openfile' or self.path == '/opendirectory'),
            save_=self.path.startswith('/savefile'),
            directory_=self.path == '/opendirectory',
            proposed_name_=self.path.removeprefix('/savefile/').replace('%20', ' ') if self.path.startswith('/savefile/') else ""
        )

        uris = list(map(str, list(dbus_result['uris']))) # Fuck you dbus

        if (len(uris) == 0):
            return self.wfile.write('No file selected'.encode())

        return self.wfile.write(uris[0].encode())
    

    def _openfiles(self):
        dbus_result = bus_call_picker(open_=True, multiple_=True)

        uris = list(map(str, list(dbus_result['uris']))) # Fuck you dbus

        if (len(uris) == 0):
            return self.wfile.write('No files selected'.encode())

        return self.wfile.write(('|'.join(uris)).encode())
    

    def _openfilemanager(self, path: str):
        bus_open_file_manager(path)
        
        return self.wfile.write('Maybe succeeded'.encode())


    def do_GET(self):
        self._set_headers()

        if (self.path == '/openfile' 
            or self.path.startswith('/savefile')
            or self.path == '/opendirectory'):

            return self._opensave()

        if (self.path == '/openfiles'):
            return self._openfiles()

        if (self.path.startswith('/openfilemanager/')):
            directory_path = self.path[len('/openfilemanager/'):]

            return self._openfilemanager(directory_path)            
            
        return self.wfile.write('Unknown method'.encode())



def already_running():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    result = sock.connect_ex(('127.0.0.1', 8912))

    running = result == 0
    sock.close()
    
    return running


def run(server_class=HTTPServer, handler_class=S, addr='127.0.0.1', port=8912):
    if already_running():
        print('Server is already running.')
        exit(-1)

    server_address = (addr, port)
    httpd = server_class(server_address, handler_class)

    print(f'Starting httpd server on {addr}:{port}')
    httpd.serve_forever()

run()