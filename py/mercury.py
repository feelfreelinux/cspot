import socket
import struct
import threading
from parameter import *
from utils import REQUEST_TYPE

try:
    import protocol.impl.mercury_pb2 as mercury
    import protocol.impl.spirc_pb2 as spirc

except:
    raise Exception("PROTO stubs were not found or have been corrupted. Please regenerate from .proto files using process_proto.py")


class MercuryManager(threading.Thread):

    def __init__(self, connection):
        super(MercuryManager, self).__init__()
        self._connection = connection
        self._sequence = 0x0000000000000000
        self._audio_key_sequence = int(0)
        self._audio_chunk_sequence = 0
        self._callbacks = {}
        self._subscriptions = {}
        self._terminated = False
        self.start()
        self._country = None
        self._audio_chunk_callback = None

    def get_country(self):
        return self._country

    def set_callback(self, seq_id, func):
        self._callbacks[seq_id] = func

    def is_terminated(self):
        return self._terminated

    def terminate(self):
        self._terminated = True

    def _process04(self, data):
        self._connection.send_packet(0x49, data)

    def _process_country_response(self, data):
        self._country = data.decode("ascii")

    def _process_audio_key(self, command, data):
        if len(data) >= 4:
            seq_id = int.from_bytes(data[:4],
                                    byteorder='big')
            try:
                callback = self._callbacks[seq_id]
                del (self._callbacks[seq_id])
            except:
                callback = None

            if callback:
                callback(command == AUDIO_KEY_SUCCESS_RESPONSE, data[4:])
            else:
                print('Callback for key %d is not found' % seq_id)
        else:
            print('Wrong format of the key response', data)

    def _parse_response(self, payload):
        header_size = struct.unpack(">H", payload[13: 15])[0]
        header = mercury.Header()
        header.ParseFromString(payload[15: 15 + header_size])
        # Now go through all parts and separate them
        pos = 15 + header_size
        parts = []
        while pos < len(payload):
            chunk_size = struct.unpack(">H", payload[pos: pos + 2])[0]
            chunk = payload[pos + 2: pos + 2 + chunk_size]
            parts.append(chunk)
            pos += 2 + chunk_size

        return int.from_bytes(payload[2: 10],
                              byteorder='big'), header, parts

    def run(self):
        while not self._terminated:
            try:
                response_code, size, payload = self._connection.recv_packet(0.1)

                if response_code == FIRST_REQUEST:
                    self._process04(payload)
                elif response_code == COUNTRY_CODE_RESPONSE:
                    self._process_country_response(payload)
                elif response_code in (AUDIO_KEY_SUCCESS_RESPONSE, AUDIO_KEY_FAILURE_RESPONSE):
                    self._process_audio_key(response_code, payload)
                elif response_code == PONG_ACK:
                    pass
                elif response_code == AUDIO_CHUNK_SUCCESS_RESPONSE or \
                        response_code == AUDIO_CHUNK_FAILURE_RESPONSE:
                    self._audio_chunk_callback and self._audio_chunk_callback(
                        success=(response_code == AUDIO_CHUNK_SUCCESS_RESPONSE),
                        payload=payload)
                elif response_code == REQUEST_TYPE.GET.as_command() or response_code == REQUEST_TYPE.SUB.as_command() or response_code == 0xb5:
                    seq_id, header, parts = self._parse_response(payload)
                    try:
                        callback = self._callbacks[seq_id]
                        del (self._callbacks[seq_id])
                    except:
                        callback = None

                    if callback:
                        callback(header, parts)
                    else:
                        subs = self._subscriptions[header.uri]
                        if subs:
                            subs(header, parts)
                        else:
                            print('Callback for', seq_id, 'is not found')

                elif response_code != INVALID_COMMAND:
                    print('Received unknown response:', hex(response_code), ' len ', size)
                    pass

                """
                    0x4a => (), 
                    0xb2...0xb6 => self.mercury().dispatch(cmd, data),
                """
            except socket.timeout:
                pass

    def execute(self, request_type, uri, callback, subscription = None, payload = []):
        header = mercury.Header(**{'uri': uri,
                                   'method': str(request_type)})
        if request_type == REQUEST_TYPE.SUB:
            self._subscriptions[uri] = subscription
        buffer = b'\x00\x08' + \
                 self._sequence.to_bytes(8, byteorder='big') + \
                 b'\x01' + \
                 struct.pack(">H", len(payload)+1) + \
                 struct.pack(">H", len(header.SerializeToString())) + \
                 header.SerializeToString()
        for i in payload:
            print("OI!!!")
            buffer += struct.pack(">H", len(i))
            buffer += i
        self.set_callback(self._sequence,
                          callback)
        self._sequence += 1
        self._connection.send_packet(request_type.as_command(),
                                     buffer)

    def request_audio_key(self, track_id, file_id, callback):
        buffer = file_id.to_bytes(20, byteorder='big') + \
                 track_id.to_bytes(16, byteorder='big') + \
                 self._audio_key_sequence.to_bytes(4, byteorder='big') + \
                 b'\x00\x00'
        self.set_callback(self._audio_key_sequence,
                          callback)
        self._audio_key_sequence += 1
        self._connection.send_packet(AUDIO_KEY_REQUEST_COMMAND,
                                     buffer)

    def fetch_audio_chunk(self, file_id, index, callback):
        sample_start = int(index * AUDIO_CHUNK_SIZE / 4)
        sample_finish = int((index + 1) * AUDIO_CHUNK_SIZE / 4)
        buffer = self._audio_chunk_sequence.to_bytes(2, byteorder='big') + \
                 b'\x00\x01' + \
                 b'\x00\x00' + \
                 b'\x00\x00\x00\x00' + \
                 b'\x00\x00\x9C\x40' + \
                 b'\x00\x02\x00\x00' + \
                 file_id.to_bytes(20, byteorder='big') + \
                 sample_start.to_bytes(4, byteorder='big') + \
                 sample_finish.to_bytes(4, byteorder='big')
        self._audio_chunk_callback = callback
        self._audio_chunk_sequence += 1
        self._connection.send_packet(AUDIO_CHUNK_REQUEST_COMMAND,
                                     buffer)
