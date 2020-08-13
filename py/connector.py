import json
from google.protobuf import text_format
import protocol.impl.authentication_pb2 as Authentication
from authtoken import AuthToken
from connection import Connection
from mercury import MercuryManager
from session import Session
from pyfy import Spotify
from blob import Blob

message = None
with open('credentials/YiP.dat', 'r') as f:
    message = f.read()
    f.close()

login = Authentication.LoginCredentials()
text_format.Merge(message, login)

blob = Blob('credentials/YiP.blob',  login.username, login.typ, login.auth_data.decode())

connection = Connection()
session = Session().connect(connection)
session.authenticate(login)
manager = MercuryManager(connection)
authToken = AuthToken(manager)
print("AuthToken: ", authToken)
manager.terminate()


token = None
with open('auth_data.dat', 'r') as f:
    data = json.load(f)
    token = data['accessToken']

print(token)

spt = Spotify(token)
devices = spt.devices()

send_to = {
    'device_ids': '97a7571d833dac43b6f5f344b90d29c9c7617678'
}
#spt.playback_transfer(send_to)


print(devices)