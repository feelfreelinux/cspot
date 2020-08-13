from zeroconf import IPVersion, ServiceInfo, Zeroconf
import socket
from time import sleep
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs
import json
import os
import binascii
import os
import binascii
from cgi import parse_header, parse_multipart
import hashlib
import ssl
random_function = ssl.RAND_bytes
import base64
PRIME_18 = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A63A3620FFFFFFFFFFFFFFFF
GENERATOR = 2
from hashlib import sha1
import hmac
import secrets

defaultInfoResponse = {
    "status": 101,
    "statusString": "OK",
    "spotifyError": 0,
    "version": "2.7.1",
    "libraryVersion": "1.2.2",
    "accountReq": "PREMIUM",
    "brandDisplayName": "librespot-org",
    "modelDisplayName": "cpot",
    "voiceSupport": "NO",
    "availability": "",
    "productID": 0,
    "tokenType": "default",
    "groupStatus": "NONE",
    "resolverVersion": "0",
    "scope": "streaming,client-authorization-universal",
    "activeUser": "",
    "deviceID": "t8s2ogzatcbtgmkyrlqpagjkbc3e5d4ucbgnhlno",
    "remoteName": "DDD speaker",
    "publicKey": "6HBKO1dTcEA4XYJpoyJES0azVBSgXeRcZBRRWzJwM7rsJfiWOiHRuSlEe/ObW84yRx0mEKhnxXOIg1GpVV3jeaK/1o0H0YQvSV8sKrrMdV0zL3LvdBmLIUIeccAGyhql",
    "deviceType": "COMPUTER"
}



class DiffieHellman:
    def __init__(self):
        self.privateKey = 0
        self.privateKey = int.from_bytes(secrets.token_bytes(95), byteorder='big')
        self.publicKey = pow(GENERATOR, self.privateKey, PRIME_18)

    def computeSharedKey(self, remoteKeyBytes):
        return pow(int.from_bytes(remoteKeyBytes, byteorder='big'), self.privateKey, PRIME_18)
        
oiKeys = DiffieHellman()

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if parse_qs(self.path)['/?action'][0] == "getInfo":
            self.getInfo()

    def do_POST(self):
        postParams = self.parse_POST()
        print("====")
        print(postParams)
        print ("===")
        self.userName = postParams[b'userName'][0].decode()
        self.blobStr = postParams[b'blob'][0].decode()
        self.clientKey = postParams[b'clientKey'][0].decode()

        print (self.clientKey)
        print(self.blobStr)


        self.clientKeyBytes = base64.decodebytes(self.clientKey.encode("utf-8"))
        self.blobBytes = base64.decodebytes(self.blobStr.encode("utf-8"))
        self.encrypted = self.blobBytes[16:-20]
        self.checksum = self.blobBytes[-20:]

        self.remoteKey = oiKeys.computeSharedKey(self.clientKeyBytes)
        self.remoteKeyBytes = self.remoteKey.to_bytes(self.remoteKey.bit_length(), 'big')
        
        self.baseKey = sha1(self.remoteKeyBytes).digest()[:16]
        self.checksumKey = hmac.new(self.baseKey, b"checksum", sha1).digest()
        self.encryptionKey = hmac.new(self.baseKey, b"encryption", sha1).digest()
        self.mac = hmac.new(self.encrypted, b"encryption", sha1).digest()
        print(self.mac)
        print(self.checksum)


    def parse_POST(self):
        ctype, pdict = parse_header(self.headers['content-type'])
        if ctype == 'multipart/form-data':
            postvars = parse_multipart(self.rfile, pdict)
        elif ctype == 'application/x-www-form-urlencoded':
            length = int(self.headers['content-length'])
            postvars = parse_qs(
                self.rfile.read(length),
                keep_blank_values=1)
        else:
            postvars = {}
        return postvars

    def getInfo(self):
        print("BRUH")
        defaultInfoResponse["publicKey"] = base64.encodebytes(oiKeys.publicKey.to_bytes(oiKeys.publicKey.bit_length(), 'big')).decode()
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        self.wfile.write(json.dumps(defaultInfoResponse,
                                    ensure_ascii=False).encode('utf-8'))


class ZeroconfServer:
    def __init__(self, port):
        self.info = ServiceInfo(
            "_spotify-connect._tcp.local.",
            "dddspot._spotify-connect._tcp.local.",
            addresses=[socket.inet_aton("127.0.0.1")],
            port=2137,
            properties={'VERSION': '1.0', 'CPATH': '/', 'Stack': "SP"},
            server="dddspot.local."
        )

        self.zeroconf = Zeroconf()
        print("Registration of a service, press Ctrl-C to exit...")
        self.zeroconf.register_service(self.info)

    def unregister(self):
        self.zeroconf.unregister_service(self.info)
        self.zeroconf.close()


d = ZeroconfServer(2137)
httpd = HTTPServer(('localhost', 2137), SimpleHTTPRequestHandler)
httpd.serve_forever()
d.unregister()
