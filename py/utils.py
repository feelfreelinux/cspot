import enum
import protocol.impl.spirc_pb2 as spirc


class REQUEST_TYPE(enum.Enum):
    SEND = 1
    GET = 2
    SUB = 3
    UNSUB = 4

    def __str__(self):
        return self.name

    def as_command(self):
        if self.name == 'SUB':
            return 0xb3
        if self.name == 'UNSUB':
            return 0xb4
        if self.name == 'SEND' or \
           self.name == 'GET':
            return 0xb2


def createInitialDeviceState():
    return spirc.DeviceState(**{
        'sw_version': '2.1.0',
        'is_active': False,
        'can_play': True,
        'volume': 32,
        'name': 'BieDAC',
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


def createInitialState():
    return spirc.State(**{
        'repeat': False,
        'shuffle': False,
        'status': spirc.kPlayStatusStop,
        'position_ms': 0,
        'position_measured_at': 0
    })
