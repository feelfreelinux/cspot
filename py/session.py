import hashlib
import hmac
import random
from parameter import *

from diffiehellman import diffiehellman

# patch DH with non RFC prime to be used for handshake
if not 1 in diffiehellman.PRIMES:
    diffiehellman.PRIMES.update({1: {
        "prime": 0xffffffffffffffffc90fdaa22168c234c4c6628b80dc1cd129024e088a67cc74020bbea63b139b22514a08798e3404ddef9519b3cd3a431b302b0a6df25f14374fe1356d6d51c245e485b576625e7ec6f44c42e9a63a3620ffffffffffffffff,
        "generator": 2
    }})

try:
    import protocol.impl.keyexchange_pb2 as keyexchange
    import protocol.impl.authentication_pb2 as authentication
except:
    raise Exception(
        "PROTO stubs were not found or have been corrupted. Please regenerate from .proto files using process_proto.py")


'''

'''


class Session:

    def __init__(self):
        # Generate local keys`
        self._local_keys = diffiehellman.DiffieHellman(
            group=1, key_length=KEY_LENGTH)
        self._local_keys.generate_private_key()
        self._local_keys.generate_public_key()

    def _sendClientHelloRequest(self):
        diffiehellman_hello = keyexchange.LoginCryptoDiffieHellmanHello(
            **{'gc': self._local_keys.public_key.to_bytes(KEY_LENGTH, byteorder='big'),
               'server_keys_known': 1})

        request = keyexchange.ClientHello(
            **{'build_info': keyexchange.BuildInfo(**{'product': keyexchange.PRODUCT_PARTNER,
                                                      'platform': keyexchange.PLATFORM_LINUX_X86,
                                                      'version': SPOTIFY_API_VERSION}),
               'cryptosuites_supported': [keyexchange.CRYPTO_SUITE_SHANNON],
               'login_crypto_hello': keyexchange.LoginCryptoHelloUnion(**{'diffie_hellman': diffiehellman_hello}),
               'client_nonce': bytes([int(random.random() * 0xFF) for x in range(0, 0x10)]),
               'padding': bytes([0x1E]),
               'feature_set': keyexchange.FeatureSet(**{'autoupdate2': True})})
        request_data = request.SerializeToString()
        return self._connection.send_packet(b"\x00\x04",
                                            request_data)

    def _processAPHelloResponse(self, init_client_packet):
        prefix, size, init_server_packet = self._connection.recv_packet()
        response = keyexchange.APResponseMessage()
        response.ParseFromString(init_server_packet[4:])

        remote_key = response.challenge.login_crypto_challenge.diffie_hellman.gs
        self._local_keys.generate_shared_secret(int.from_bytes(remote_key,
                                                               byteorder='big'))
        mac_original = hmac.new(self._local_keys.shared_secret.to_bytes(KEY_LENGTH,
                                                                        byteorder='big'),
                                digestmod=hashlib.sha1)
        data = []
        for i in range(1, 6):
            mac = mac_original.copy()
            mac.update(init_client_packet + init_server_packet + bytes([i]))
            digest = mac.digest()
            data += digest

        mac = hmac.new(bytes(data[:0x14]),
                       digestmod=hashlib.sha1)
        mac.update(init_client_packet + init_server_packet)
        return (mac.digest(),
                bytes(data[0x14: 0x34]),
                bytes(data[0x34: 0x54]))

    """ Send handsheke challenge """

    def _sendClientHandshakeChallenge(self,
                                      challenge):
        diffie_hellman = keyexchange.LoginCryptoDiffieHellmanResponse(
            **{'hmac': challenge})
        crypto_response = keyexchange.LoginCryptoResponseUnion(
            **{'diffie_hellman': diffie_hellman})
        packet = keyexchange.ClientResponsePlaintext(**{'login_crypto_response': crypto_response,
                                                        'pow_response': {},
                                                        'crypto_response': {}})
        self._connection.send_packet(prefix=None,
                                     data=packet.SerializeToString())

    def connect(self, connection):
        self._connection = connection
        # self._connection.connect()
        init_client_packet = self._sendClientHelloRequest()
        challenge, send_key, recv_key = self._processAPHelloResponse(
            init_client_packet)
        self._sendClientHandshakeChallenge(challenge)
        self._connection.handshake_completed(send_key,
                                             recv_key)
        return self

    def authenticate(self):

        auth_request = authentication.ClientResponseEncrypted(**{'login_credentials': authentication.LoginCredentials(**{'username':      "d",
                                                                                                                         'typ':           authentication.AUTHENTICATION_USER_PASS,
                                                                                                                         'auth_data':     bytes("d", 'ascii')}),
                                                                 'system_info': authentication.SystemInfo(**{'cpu_family':                authentication.CPU_UNKNOWN,
                                                                                                             'os':                        authentication.OS_UNKNOWN,
                                                                                                             'system_information_string': INFORMATION_STRING,
                                                                                                             'device_id':                 DEVICE_ID}),
                                                                 'version_string': VERSION_STRING})
        # auth_request = authentication.ClientResponseEncrypted(
        #     **{'system_info': authentication.SystemInfo(**{'cpu_family': authentication.CPU_UNKNOWN,
        #                                                    'os': authentication.OS_UNKNOWN,
        #                                                    'system_information_string': INFORMATION_STRING,
        #                                                    'device_id': DEVICE_ID}),
        #        'version_string': VERSION_STRING})
        # auth_request.login_credentials.CopyFrom(login_credentials)

        packet = self._connection.send_packet(LOGIN_REQUEST_COMMAND,
                                              auth_request.SerializeToString())
        # Get response
        command, size, body = self._connection.recv_packet()
        if command == AUTH_SUCCESSFUL_COMMAND:
            auth_welcome = authentication.APWelcome()
            auth_welcome.ParseFromString(body)
            print(auth_welcome)
            return auth_welcome.reusable_auth_credentials_type, auth_welcome.reusable_auth_credentials
        elif command == AUTH_DECLINED_COMMAND:
            raise Exception('AUTH DECLINED. Code: %02X' % command)

        raise Exception('UNKNOWN AUTH CODE %02X' % command)
