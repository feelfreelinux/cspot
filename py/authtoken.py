import json
import os
import threading
import time

from parameter import *

from utils import REQUEST_TYPE

class AuthToken:

    def __init__(self, mercury_manager, client_id, scope=DEFAULT_SCOPE):
        self._mercury_manager = mercury_manager
        self._event = threading.Event()
        self._event.clear()

        self._mercury_manager.execute(REQUEST_TYPE.GET,
                                      AUTH_TOKEN_TEMPLATE.format(clientId=client_id, scope=scope),
                                      self._info_callback)
        self._event.wait()

    def _info_callback(self, header, parts):
        data = parts[0].decode('utf-8')
        token_data = json.loads(data)
        self.accessToken = token_data['accessToken']
        self.expiresIn = token_data['expiresIn']
        self.tokenType = token_data['tokenType']
        self.scope = token_data['scope']
        self.expiresAt = time.time()
        #self.save()
        # open( 'auth_data.dat', 'wb' ).write( parts[ 0 ] )
        self._event.set()

    def load(self, path='auth_data.dat'):
        os.path.exists(path)

    def save(self, path='auth_data.dat'):
        data = {
            'accessToken': self.accessToken,
            'expiresIn': self.expiresIn,
            'expiresAt': self.expiresAt,
            'tokenType': self.tokenType,
            'scope': self.scope
        }
        with open(path, 'w') as f:
            json.dump(data, f)
            f.close()
