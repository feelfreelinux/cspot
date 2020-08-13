from blob import Blob
from connection import Connection
from session import Session
from mercury import MercuryManager
from spirc import SpircController
import time
from spotify import Track
import protocol.impl.metadata_pb2 as metadata
import pyaes
from parameter import *

# blob = Blob()
# blob.load("auth.dat")
# login = blob.create_login()

connection = Connection()
session = Session().connect(connection)
session.authenticate()


manager = MercuryManager(connection)
time.sleep(5)

spircController = SpircController(manager)
track = Track(manager, bytes('4MbUcaX58NkUmKXe6FuBaV', 'ascii'))
if not track.load(metadata.AudioFile.Format.Value('OGG_VORBIS_320')):
    track = None
else:
    print(track._track)
    print(track)
time.sleep(5)
# if track:
#     print('Found file matching selected format. fileId= %s' %(hex(track._file_id), ))
#     # Now load some audio data from track
#     start_time = time.time()
#     aes = pyaes.AESModeOfOperationCTR(track._audio_key.to_bytes(16, byteorder='big'), pyaes.Counter(int.from_bytes(AUDIO_AESIV, byteorder='big')))
#     f = open('dupa.ogg', 'wb')
#     for i in range(10000):
#         chunk_data = track.get_chunk(i)
#         if i == 0:
#             print('File chunk #%d received. Size %d. Header %s' %(i, len(chunk_data), track._chunk_header))
#         else:
#             print('File chunk #%d received. Size %d' %(i, len(chunk_data)))
#         decrypted_chunk = aes.decrypt(chunk_data)
#         f.write(decrypted_chunk)
#         if len(chunk_data) < AUDIO_CHUNK_SIZE:
#             print('Finished in %d secs' % (time.time() - start_time))
#             f.close()