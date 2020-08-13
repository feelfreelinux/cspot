import hmac
from hashlib import sha1
import base64
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
PRIME_18 = 0xFFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A63A3620FFFFFFFFFFFFFFFF
GENERATOR = 2
clientKey = "5qME2N1wKiZ7HEhU7BcdaqysESoHb8eGuR9ZFmkOIQQul845PclARwRbadaiWmdnfvfAaZrUmAeNBVG2zfIkvt8yz+OnW7nKBF8CV2gL9PYJ+3qtzDbVjVy2J+8p4Ef6"
blobStr = "sLNXlUqRBOvg+3UmJF5Kpfw08zJnw7W6/Cqar3lVHgAznMWgfo0MRU0deH4gzxIfDqWhKPTNFAHypvqQZHS3lCuPNfjAli84k7oAgIbyNN6Yksc70Te/YNsNAsLilin5431vT6m0Pmq8WRH8i/kdvY/KxQBwkYaii3zRX7/P8kJrzLLQwVOEFkWOQsECaVpKxgpqHrudC2Zrc8hk0EzDt1kmvBKj/zuVM5pkJtdb/ZdHJ4934ZeEVmQ0Nw/5ZHdhk1IsQkwgO/ZEGe8OR2CtG+udznLkJv/NXtYTsu84dXOvGoLAnDZmfmzQpi6JgucUCeqVWc/gC62jRzy0iq3RO6MxdCZwYf1DxEVkxzek/Qw="

privateKey = 0
_bytes = 95
while(privateKey.bit_length() < 95):
    privateKey = int.from_bytes(random_function(_bytes), byteorder='big')
clientKeyBytes = base64.decodebytes(clientKey.encode("utf-8"))
blobBytes = base64.decodebytes(blobStr.encode("utf-8"))
encrypted = blobBytes[16:-20]
checksum = blobBytes[-20:]

remoteKey = pow(int.from_bytes(clientKeyBytes, byteorder='big'), privateKey, PRIME_18)
remoteKeyBytes = remoteKey.to_bytes(remoteKey.bit_length(), 'big')

baseKey = sha1(remoteKeyBytes).digest()[:16]
checksumKey = hmac.new(baseKey, b"checksum", sha1).digest()
encryptionKey = hmac.new(baseKey, b"encryption", sha1).digest()
mac = hmac.new(encrypted, b"encryption", sha1).digest()

print(mac)
print(checksum)
# print(clientKey)
