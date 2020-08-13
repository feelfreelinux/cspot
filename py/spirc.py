from mercury import MercuryManager
import protocol.impl.spirc_pb2 as spirc
from parameter import *
from utils import REQUEST_TYPE, createInitialDeviceState, createInitialState
import time


class SpircController:
    seqNum = 0
    initialDeviceState = createInitialDeviceState()
    initialState = createInitialState()

    def __init__(self, mercury_manager):
        self.mercury_manager = mercury_manager
        self.mercury_manager.execute(REQUEST_TYPE.SUB,
                                     "hm://remote/user/fliperspotify/",
                                     self._subscribed_callback, subscription=self._handle_frame)
        print("oo")

    def _subscribed_callback(self, header, parts):
        self._send_cmd(None, spirc.kMessageTypeHello)
        print("Subscribed")

    def _notify(self):
        self._send_cmd(None, spirc.kMessageTypeNotify)

    def _handle_frame(self, header, parts):
        fram = spirc.Frame()
        fram.ParseFromString(parts[0])
        print("Got frame!")
        if fram.typ is spirc.kMessageTypeLoad:
            print("Load frame")
            self.initialDeviceState.is_active = True
            self.initialDeviceState.became_active_at = int(round(time.time() * 1000))
            self.initialState.playing_track_index = fram.state.playing_track_index
            self.initialState.status = spirc.kPlayStatusPlay
            del self.initialState.track[:]
            self.initialState.track.extend(fram.state.track)
            self.initialState.context_uri = fram.state.context_uri
            self._notify()

    def _frame_sent(self, header, parts):
        print("Frame sent! ", parts)

    def _send_cmd(self, recipient, messageType):
        singleFrame = spirc.Frame(**{'version': 1,
                                     'ident': DEVICE_ID,
                                     'protocol_version': "2.0.0",
                                     'device_state': self.initialDeviceState,
                                     'seq_nr': self.seqNum,
                                     'state': self.initialState,
                                     'typ': messageType,
                                     'state_update_id': int(round(time.time() * 1000)),
                                     'recipient': recipient})
        self.seqNum += 1

        request_data = singleFrame.SerializeToString()
        self.mercury_manager.execute(
            REQUEST_TYPE.SEND, "hm://remote/user/fliperspotify/", self._frame_sent, payload=[request_data])