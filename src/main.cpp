#include <stdio.h>
#include <iostream>
#include <string.h>
#include <PlainConnection.h>
#include <Session.h>



int main()
{    
    auto connection = new PlainConnection();
    connection->connectToAp();

    auto session = new Session();
    session->connect(connection);

    return 0;
}
//   def _sendClientHelloRequest( self ):
//     diffiehellman_hello= keyexchange.LoginCryptoDiffieHellmanHello( **{ 'gc':                self._local_keys.public_key.to_bytes( KEY_LENGTH, byteorder='big' ), \
//                                                                         'server_keys_known': 1 } )                                            

//     request = keyexchange.ClientHello( **{ 'build_info':             keyexchange.BuildInfo( **{ 'product':  keyexchange.PRODUCT_PARTNER, 
//                                                                                                 'platform': keyexchange.PLATFORM_LINUX_X86, 
//                                                                                                 'version':  SPOTIFY_API_VERSION } ),
//                                            'cryptosuites_supported': [ keyexchange.CRYPTO_SUITE_SHANNON ],
//                                            'login_crypto_hello':     keyexchange.LoginCryptoHelloUnion( **{ 'diffie_hellman': diffiehellman_hello } ),
//                                            'client_nonce' :          bytes( [ int( random.random()* 0xFF ) for x in range( 0, 0x10 ) ] ),
//                                            'padding':                bytes( [ 0x1E ] ),
//                                            'feature_set':            keyexchange.FeatureSet( **{ 'autoupdate2': True } ) } )
//     request_data= request.SerializeToString()
//     return self._connection.send_packet( b"\x00\x04", 
//                                          request_data  )