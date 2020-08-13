
KEY_LENGTH=               96

SPOTIFY_AP_ENDPOINT = 'http://apresolve.spotify.com'
SPOTIFY_AP_ADDRESS=       ( 'gew1-accesspoint-b-vlvq.ap.spotify.com', 4070 )
SPOTIFY_AP_FALLBACK = ('ap.spotify.com', 80)



SPOTIFY_API_VERSION=      0x10800000000
INFORMATION_STRING=       "pyspotify"

DEVICE_ID=                "452198fd329622876e14907634264e6f332e5410"
VERSION_STRING=           "2.1.0"
REMOTE_NAME_DEFAULT = "Spotipy"


LOGIN_REQUEST_COMMAND=        0xAB
AUTH_SUCCESSFUL_COMMAND=      0xAC
AUTH_DECLINED_COMMAND=        0xAD

FIRST_REQUEST=                0x04
PONG_ACK = 0x4a
AUDIO_CHUNK_REQUEST_COMMAND=  0x08
AUDIO_CHUNK_SUCCESS_RESPONSE= 0x09
AUDIO_CHUNK_FAILURE_RESPONSE= 0x0A
AUDIO_KEY_REQUEST_COMMAND=    0x0C
AUDIO_KEY_SUCCESS_RESPONSE=   0x0D
AUDIO_KEY_FAILURE_RESPONSE=   0x0E
COUNTRY_CODE_RESPONSE=        0x1B

MAC_SIZE=                     4
HEADER_SIZE=                  3
MAX_READ_COUNT=               5
INVALID_COMMAND=              0xFFFF
AUDIO_CHUNK_SIZE=             0x20000

AUDIO_AESIV=                  b'\x72\xe0\x67\xfb\xdd\xcb\xcf\x77\xeb\xe8\xbc\x64\x3f\x63\x0d\x93'
BASE62_DIGITS=                b'0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'
TRACK_PATH_TEMPLATE=          'hm://metadata/3/track/%s'
ALBUM_PATH_TEMPLATE=          'hm://metadata/3/album/%s'
ARTIST_PATH_TEMPLATE=         'hm://metadata/3/artist/%s'
USER_PATH_TEMPLATE=           'hm://identity/v1/user/%s'
PLAYLISTS_PATH_TEMPLATE=      'hm://playlist/user/%s/rootlist'
PLAYLIST_CONTENTS_TEMPLATE=   'hm://playlist/%s?from=0&length=100'
EPISODE_PATH_TEMPLATE=        'hm://metadata/4/episode/%s'


DEFAULT_SCOPE = ','.join(["user-read-private",
            "user-library-read",
            "user-library-modify",
            "playlist-read-private",
            "playlist-modify-public",
            "playlist-modify-private"])

AUTH_TOKEN_TEMPLATE = 'hm://keymaster/token/authenticated?client_id={clientId}&scope={scope}'

ALBUM_GID=  ''
ARTIST_GID= ''