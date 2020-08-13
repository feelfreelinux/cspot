import enum
import random
import socket
import struct
import shannon
import requests
from parameter import *


class Connection:
  class CONNECT_TYPE(enum.Enum):
    CONNECT_TYPE_HANDSHAKE= 1
    CONNECT_TYPE_STREAM=    2

  def __init__( self ):
    self._socket = socket.socket(socket.AF_INET, 
                                socket.SOCK_STREAM)
    self._socket.connect( SPOTIFY_AP_ADDRESS )
    self._connect_type= Connection.CONNECT_TYPE.CONNECT_TYPE_HANDSHAKE
    self._partial_buffer= b''

  def _try_recv( self, size ):
    result= self._partial_buffer
    read_count= 0
    while read_count< MAX_READ_COUNT and len( result )< size:
      try:
        result+= self._socket.recv( size- len( result ) )
      except socket.timeout:
        read_count+= 1
      
    if len( result )< size:
      self._partial_buffer= result
    else:
      self._partial_buffer= b''
    
    return result

  def send_packet( self, 
                   prefix, 
                   data ):
    if prefix== None:
      prefix= b""
    
    if self._connect_type== Connection.CONNECT_TYPE.CONNECT_TYPE_HANDSHAKE:
      size=    len( prefix )+ 4+ len( data )
      request= prefix+ struct.pack(">I", size )+ data
    else:
      self._encoder.set_nonce( self._encoder_nonce )
      self._encoder_nonce+= 1

      request= bytes( [ prefix ] )+ struct.pack(">H", len( data ) )+ data
      request= self._encoder.encrypt( request )
      request+= self._encoder.finish( MAC_SIZE )

    self._socket.send( request )
    return request

  def recv_packet( self, timeout= 0 ):
    if timeout:
      self._socket.setblocking( 0 )
      self._socket.settimeout( timeout )
    else:
      self._socket.setblocking( 1 )
      
    if self._connect_type== Connection.CONNECT_TYPE.CONNECT_TYPE_HANDSHAKE:
      size_packet= self._socket.recv( 4 )
      size= struct.unpack( ">I",  size_packet )
      return '', size[ 0 ], size_packet+ self._socket.recv( size[ 0 ]- 4 )
    else:
      command= INVALID_COMMAND
      size= 0
      body= b''
      if self._partial_buffer:
        recv_header= self._partial_buffer[ :HEADER_SIZE ]
      else:
        recv_header= self._try_recv( HEADER_SIZE )
      
      if len( recv_header )== HEADER_SIZE:
        if not self._partial_buffer:
          self._decoder.set_nonce( self._decoder_nonce )
          self._decoder_nonce+= 1
          header= self._decoder.decrypt( recv_header )
          partial_size= 0
        else:
          partial_size= len( self._partial_buffer )- HEADER_SIZE
        
        size= struct.unpack( ">H", header[ 1: ] )[ 0 ]
        command= header[ 0 ]
        recv_body= self._try_recv( size- partial_size )
        if len( recv_body )+ partial_size== size:
          mac= self._try_recv( MAC_SIZE )
          if len( mac )== MAC_SIZE:
            body= self._decoder.decrypt( recv_body )+ self._partial_buffer[ HEADER_SIZE: ]
            calculated_mac= self._decoder.finish( MAC_SIZE )
            if calculated_mac!= mac:
              raise Exception( 'RECV MAC not matching', calculated_mac, mac )
          else:
            self._partial_buffer= recv_header+ recv_body+ self._partial_buffer
        else:
          self._partial_buffer= recv_header+ self._partial_buffer
          print( 'Size is not matching expected length %d vs %d' % ( size, len( recv_body )) )
      
      return command, size, body

  def handshake_completed( self, send_key, recv_key ):
    self._connect_type= Connection.CONNECT_TYPE.CONNECT_TYPE_STREAM
    
    # Generate shannon streams
    self._encoder_nonce= 0
    self._encoder= shannon.Shannon( send_key )
    
    self._decoder_nonce= 0
    self._decoder= shannon.Shannon( recv_key )
    
