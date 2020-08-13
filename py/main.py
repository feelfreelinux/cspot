from time import sleep
from zeroauth import Zeroauth
import json
from authtoken import AuthToken
from connection import Connection
from mercury import MercuryManager
from session import Session
import pyaes
import time

#from pyfy import Spotify
from blob import Blob
import base64
from parameter import *
from utils import REQUEST_TYPE
import os
import \
    sys,        \
    subprocess, \
    socket,     \
    struct,     \
    inspect,    \
    random,     \
    hmac,       \
    hashlib,    \
    enum,       \
    threading,  \
    time,       \
    base64
import shannon
import \
    pyaes
from authtoken import AuthToken
from connection import Connection
from mercury import MercuryManager
from session import Session


try:
    import protocol.impl.keyexchange_pb2 as keyexchange
    import protocol.impl.authentication_pb2 as authentication
    import protocol.impl.metadata_pb2 as metadata
    import protocol.impl.playlist_pb2 as spotify_playlist
    import protocol.impl.spirc_pb2 as spirc
except:
    raise Exception(
        "PROTO stubs were not found or have been corrupted. Please regenerate from .proto files using process_proto.py")


"""
  Convert from 62 symbol alphabet -> 16 symbols
"""


def _toBase16(id62):
    result = 0x00000000000000000000000000000000
    for c in id62:
        result = result * 62 + BASE62_DIGITS.find(bytes([c]))

    return result

# ----------------------------------- Track metadata and stream ----------------------------------


class Track:
    def __init__(self, mercury_manager, track):
        self._mercury_manager = mercury_manager
        self.track = str(track, 'ascii')
        self._track_id = _toBase16(track)
        print("Track id in HEX:", hex(self._track_id))
        self._file_id = None
        self._audio_key = None
        self._track = metadata.Track()
        self._event = threading.Event()

    def _audio_key_callback(self, success, payload):
        if success:
            self._audio_key = int.from_bytes(payload[:16],
                                             byteorder='big')
            print('Key received 0x%X' % self._audio_key)
        else:
            print('Key cannot be processed', payload)
        self._event.set()

    def _track_info_callback(self, header, parts):
        self._track.ParseFromString(parts[0])
        self._event.set()

    def _track_chunk_callback(self, success, payload):
        if success:
            seq = int.from_bytes(payload[:2],
                                 byteorder='big')
            if len(self._chunk_header) == 0:
                # First packet, parse header
                header_len = int.from_bytes(payload[2:4],
                                            byteorder='big')
                self._chunk_header = payload[4: header_len + 4]
            else:
                self._chunk_data += payload[2:]
                if len(payload) == 2:   # Last packet is always 2 bytes (sequence only)
                    self._event.set()
        else:
            print('Failure:', payload)
            self._event.set()

    def load(self, format):
        global ALBUM_GID,  \
            ARTIST_GID
        self._event.clear()
        self._mercury_manager.execute(REQUEST_TYPE.GET,
                                      TRACK_PATH_TEMPLATE % hex(
                                          self._track_id)[2:],
                                      self._track_info_callback)
        # Parse restrictions and alternatives
        self._event.wait()
        # Get artist and album
        ALBUM_GID = track._track.album.gid
        ARTIST_GID = track._track.album.artist[0].gid
        restriction = track._track.restriction[0]
        files = self._track.file

        # Scan through all files and match the format desired
        for file in files:
            if file.format == format:
                self._file_id = int.from_bytes(file.file_id,
                                               byteorder='big')
                self._event.clear()
                print('Requesting key for track %x file %x' %
                      (self._track_id, self._file_id))
                self._mercury_manager.request_audio_key(self._track_id,
                                                        self._file_id,
                                                        self._audio_key_callback)
                self._event.wait()
                return True

        return False

    def get_chunk(self, chunk):
        # Reset lock just in case
        self._chunk_data = b''
        self._chunk_header = b''
        self._event.clear()
        self._mercury_manager.fetch_audio_chunk(self._file_id,
                                                chunk,
                                                self._track_chunk_callback)
        self._event.wait()
        return self._chunk_data


class BlobListener:
    def __init__(self):
        self.blob = None

    def new_blob_status(self, blob: Blob):
        self.blob = blob
        print("User %s blob received" % self.blob.username)


seqNum = 0
global manager
manager = None
global initialState
initialState = spirc.State(**{
    'repeat': False,
    'shuffle': False,
    'status': spirc.kPlayStatusStop,
    'position_ms': 0,
    'position_measured_at': 0
})

global initialDeviceState
initialDeviceState = spirc.DeviceState(**{
    'sw_version': '2.1.0',
    'is_active': False,
    'can_play': True,
    'volume': 32,
    'name': 'JBL 523',
    'capabilities': [
        spirc.Capability(**{
            'typ': spirc.kCanBePlayer,
            'intValue': [1]
        }),
        spirc.Capability(**{
            'typ': spirc.kDeviceType,
            'intValue': [4]
        }),
        spirc.Capability(**{
            'typ': spirc.kGaiaEqConnectId,
            'intValue': [1]
        }),
        spirc.Capability(**{
            'typ': spirc.kSupportsLogout,
            'intValue': [0]
        }),                        spirc.Capability(**{
            'typ': spirc.kIsObservable,
            'intValue': [1]
        }),
        spirc.Capability(**{
            'typ': spirc.kVolumeSteps,
            'intValue': [64]
        }), spirc.Capability(**{
            'typ': spirc.kSupportedContexts,
            'stringValue': ["album",
                            "playlist",
                            "search",
                            "inbox",
                            "toplist",
                            "starred",
                            "publishedstarred",
                            "track"]
        }),
        spirc.Capability(**{
            'typ': spirc.kSupportedTypes,
            'stringValue': ["audio/local",
                            "audio/track",
                            "local",
                            "track"]
        }),
    ]

})


def callbacko(header, parts):
    print(parts)
    sendCmd(None, spirc.kMessageTypeHello)
    print("callbacko")

def notify():
    sendCmd(None, spirc.kMessageTypeNotify)

def kok(header, parts):
    print(parts)
    print("sendo finished")


def handleFrame(header, parts):
    print("Got frame update!")
    fram = spirc.Frame()
    fram.ParseFromString(parts[0])

    print("Frame type ", fram.typ)
    if fram.typ is spirc.kMessageTypeLoad:
        initialDeviceState.is_active = True
        initialDeviceState.became_active_at = int(round(time.time() * 1000))
        initialState.playing_track_index = fram.state.playing_track_index
        initialState.status = spirc.kPlayStatusPlay
        del initialState.track[:]
        initialState.track.extend(fram.state.track)
        initialState.context_uri = fram.state.context_uri
        notify()


def sendCmd(recipient, msgType):
    global seqNum
    seqNum += 1
    singleFrame = spirc.Frame(**{'version': 1,
                                 'ident': DEVICE_ID,
                                 'protocol_version': "2.0.0",
                                 'device_state': initialDeviceState,
                                 'seq_nr': seqNum,
                                 'state': initialState,
                                 'typ': msgType,
                                 'state_update_id': int(round(time.time() * 1000)),
                                 'recipient': recipient})

    request_data = singleFrame.SerializeToString()
    global manager
    manager.execute(REQUEST_TYPE.SEND,
                    "hm://remote/user/fliperspotify/",
                    kok, payload=[request_data])


if __name__ == "__main__":
    print("Start ZeroConf Server")
    # za = Zeroauth()
    # za.start()
    # blob_listener = BlobListener()
    # za.register_listener(blob_listener)

    # while True:
    #     sleep(1)
    #     if blob_listener.blob is not None:
    #         break
    blob = Blob()
    blob.load("auth.dat")
    # za.stop()
    login = blob.create_login()
    connection = Connection()
    session = Session().connect(connection)
    session.authenticate(login)
    manager = MercuryManager(connection)
    track = Track(manager, bytes('3j50HCDAs7iPL1SSJiLUK4', 'ascii'))
    if not track.load(metadata.AudioFile.Format.Value('OGG_VORBIS_320')):
        track = None
    else:
        print(track._track)

    manager.execute(REQUEST_TYPE.SUB,
                    "hm://remote/user/fliperspotify/",
                    callbacko, subscription=handleFrame)
    sleep(10)
    if track:
        print('Found file matching selected format. fileId= %s' %(hex(track._file_id), ))
        # Now load some audio data from track
        start_time = time.time()
        aes = pyaes.AESModeOfOperationCTR(track._audio_key.to_bytes(16, byteorder='big'),pyaes.Counter(int.from_bytes(AUDIO_AESIV, byteorder='big')))
        f = open('dupa.ogg', 'wb')
        for i in range(10000):
            chunk_data = track.get_chunk(i)
            if i == 0:
                print('File chunk #%d received. Size %d. Header %s' % (i, len(chunk_data), track._chunk_header))
            else:
                print('File chunk #%d received. Size %d' % (i, len(chunk_data)))
            decrypted_chunk = aes.decrypt(chunk_data)
            f.write(decrypted_chunk)
            if len(chunk_data) < AUDIO_CHUNK_SIZE:
                print('Finished in %d secs' % (time.time() - start_time))
                f.close()
    # client_id = '<YOUR_CLIENT_ID>'
    # authToken = AuthToken(manager, client_id)
    # manager.terminate()
    #token = authToken.accessToken
    #print("AuthToken: ", token)

    #spt = Spotify(token)
    #devices = spt.devices()
    # print(devices)

    #last_10_played = spt.recently_played_tracks(10)
    # print(last_10_played)
