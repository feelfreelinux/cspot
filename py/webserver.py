import random
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from urllib.parse import urlparse, parse_qs
import json
from google.protobuf import text_format
from blob import Blob
import base64
from diffiehellman import diffiehellman as dh
from parameter import *
from time import sleep

# patch DH with non RFC prime to be used for handshake
if not 1 in dh.PRIMES:
    dh.PRIMES.update({1: {
        "prime": 0xffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a63a3620ffffffffffffffff,
        "generator": 2
    }})
signedIn = False

class RequestHandler(BaseHTTPRequestHandler):
    '''Static keys'''
    keys = dh.DiffieHellman(group=1, key_length=KEY_LENGTH)
    keys.generate_private_key()
    keys.generate_public_key()
    publicKey = keys.public_key

    def __init__(self, request, client_address, server) -> None:
        super().__init__(request, client_address, server)

    def _set_200_headers(self, keyword, value):
        self.send_response(200)
        # self.send_header('Content-type', 'application/json')
        self.send_header(keyword, value)
        self.end_headers()

    def _set_404_headers(self):
        self.send_response(404)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

    def do_GET(self):
        parse = urlparse(self.path)
        request_path = parse.path
        query = parse.query

        key_byte_array = self.publicKey.to_bytes((self.publicKey.bit_length() + 7) // 8, 'big')
        b64_key = base64.standard_b64encode(key_byte_array).decode()

        if query == "action=getInfo":
            self._set_200_headers('Content-type', 'application/json')
            self.wfile.write(json.dumps(
                {
                    "status": 101,
                    "statusString": "ERROR-OK",
                    "spotifyError": 0,
                    "version": VERSION_STRING,
                    "deviceID": DEVICE_ID,
                    "remoteName": REMOTE_NAME_DEFAULT,
                    "activeUser": "fliperspotify" if signedIn else "",
                    "publicKey": b64_key,
                    "deviceType": "SPEAKER",
                    "libraryVersion": "0.1.0",
                    "accountReq": "PREMIUM",
                    "brandDisplayName": "librespot",
                    "modelDisplayName": "librespot"
                }).encode())
        else:
            self._set_404_headers()

    # POST echoes the message adding a JSON field
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])  # <--- Gets the size of data
        data_bytes = self.rfile.read(content_length)  # <--- Gets the data itself
        data_str = data_bytes.decode('utf-8')
        path = self.path
        headers = self.headers
        var = parse_qs(data_str)
        print("Var: ", var)

        if 'action' not in var or 'addUser' not in var['action']:
            self._set_404_headers()
            return

        user_name = var['userName'][0].encode()
        blob_bytes = var['blob'][0].encode()
        client_key = var['clientKey'][0].encode()
        device_name = var['deviceName'][0]

        response = json.dumps(
            {
                "status": 101,
                "spotifyError": 0,
                "statusString": "ERROR-OK"
            }).encode()

        self._set_200_headers('Content-Length', str(len(response)))
        self.wfile.write(response)

        blob = Blob()
        print(blob_bytes)
        print(client_key)
        print(user_name)
        print(device_name)
        login = blob.decrypt(self.keys, blob_bytes, client_key, user_name, DEVICE_ID)
        global signedIn
        signedIn = True

        self.server.notify_listener(blob)

        with open('auth.dat', 'w') as f:
            f.write(text_format.MessageToString(login))
            f.close()

        '''
        connection = Connection()
        session = Session().connect(connection)
        reusable_token = session.authenticate(login)
        print("Token: ", reusable_token)
        manager = MercuryManager(connection)
        authToken = AuthToken(manager)
        print("AuthToken: ", authToken)
        manager.terminate()
        '''


class MyHttpServer(HTTPServer):

    def __init__(self, server_address, RequestHandlerClass):
        self.listener = None
        super().__init__(server_address, RequestHandlerClass)

    def notify_listener(self, blob):
        if self.listener is not None:
            self.listener.new_blob_status(blob)

    def register_listener(self, listener):
        self.listener = listener


class HTTPServerController:

    def __init__(self, port):
        self.port = port
        self.server_address = ('', port)
        self.httpServer = MyHttpServer(self.server_address, RequestHandler)
        self.register_listener = self.httpServer.register_listener

    def start_server_thread(self):
        thread = threading.Thread(target=self.httpServer.serve_forever)
        thread.deamon = False
        thread.start()
        print("---Server started (Port %d) ---" % self.port)
        return self.httpServer

    def stop_server_thread(self):
        self.httpServer.shutdown()


def get_random_port():
    min = 1024
    max = 65536
    randPort = random.randint(min, max)
    return randPort


if __name__ == "__main__":
    print("Start Webserver")
    random_port = 60995
    server_controller = HTTPServerController(random_port)
    server_controller.start_server_thread()
    print("Started Webserver")

    try:
        while True:
            sleep(0.1)
    except KeyboardInterrupt:
        pass
    finally:
        server_controller.stop_server_thread()
        print("Stop Webserver")
