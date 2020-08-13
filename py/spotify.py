import os,         \
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
import shannon,    \
       pyaes
from authtoken import AuthToken
from connection import Connection
from mercury import MercuryManager
from session import Session
from blob import Blob
from parameter import *
from utils import REQUEST_TYPE



try:
  import protocol.impl.keyexchange_pb2 as keyexchange
  import protocol.impl.authentication_pb2 as authentication
  import protocol.impl.metadata_pb2 as metadata
  import protocol.impl.playlist_pb2 as spotify_playlist
except:
  raise Exception( "PROTO stubs were not found or have been corrupted. Please regenerate from .proto files using process_proto.py" ) 



"""
  Convert from 62 symbol alphabet -> 16 symbols
"""
def _toBase16( id62 ):
  result= 0x00000000000000000000000000000000
  for c in id62:
    result= result* 62+ BASE62_DIGITS.find( bytes( [c] ) )
    
  return result


# ----------------------------------- Track metadata and stream ----------------------------------
class Track:
  def __init__( self, mercury_manager, track ):
    self._mercury_manager= mercury_manager
    self.track= str( track, 'ascii' )
    self._track_id= _toBase16( track )
    print( "Track id in HEX:", hex( self._track_id ) )
    self._file_id= None
    self._audio_key= None
    self._track= metadata.Track()
    self._event= threading.Event()

  def _audio_key_callback( self, success, payload ):
    if success:
      self._audio_key= int.from_bytes(payload[ :16 ], 
                                      byteorder='big')
      print( 'Key received 0x%X' % self._audio_key )
    else:
      print( 'Key cannot be processed', payload )
    self._event.set()
    
  def _track_info_callback( self, header, parts ):
    self._track.ParseFromString( parts[ 0 ] )
    self._event.set()
  
  def _track_chunk_callback( self, success, payload ):
    if success:
      seq= int.from_bytes( payload[ :2 ], 
                           byteorder='big')
      if len( self._chunk_header )== 0:
        # First packet, parse header
        header_len= int.from_bytes( payload[ 2:4 ], 
                                    byteorder='big')
        self._chunk_header= payload[ 4: header_len+ 4 ]
      else:
        self._chunk_data+= payload[ 2: ]
        if len( payload )== 2:   # Last packet is always 2 bytes (sequence only)
          self._event.set()
    else:
      print( 'Failure:', payload )
      self._event.set()
    
  def load( self, format ):
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   TRACK_PATH_TEMPLATE % hex( self._track_id )[ 2: ],
                                   self._track_info_callback )
    # Parse restrictions and alternatives
    self._event.wait()
    # Get artist and album
    # restriction= self.track._track.restriction[ 0 ]
    files= self._track.file

    # Scan through all files and match the format desired
    for file in files:
      if file.format== format:
        self._file_id= int.from_bytes( file.file_id, 
                                       byteorder='big' )
        self._event.clear()
        print( 'Requesting key for track %x file %x' % ( self._track_id, self._file_id ) )
        self._mercury_manager.request_audio_key( self._track_id, 
                                                 self._file_id,
                                                 self._audio_key_callback )
        self._event.wait()
        return True
        
    return False

  def get_chunk( self, chunk ):
    # Reset lock just in case
    self._chunk_data= b''
    self._chunk_header= b''
    self._event.clear()
    self._mercury_manager.fetch_audio_chunk( self._file_id, 
                                             chunk,
                                             self._track_chunk_callback )
    self._event.wait()
    return self._chunk_data
    
# ----------------------------------- Podcast ----------------------------------
class Episode( Track ):
  def __init__( self, mercury_manager, episode ):
    super( Episode, self ).__init__( mercury_manager, episode )
    self._episode= metadata.Episode()
    self.episode= str( episode, 'ascii' )
    
  def _episode_info_callback( self, header, parts ):
    open( 'episode.dat', 'wb' ).write( parts[ 0 ] )
    self._episode.ParseFromString( parts[ 0 ] )
    self._event.set()

  def load( self, format ):
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   EPISODE_PATH_TEMPLATE % hex( self._track_id )[ 2: ].lower(),
                                   self._episode_info_callback )
    # Parse restrictions and alternatives
    self._event.wait()
    files= self._episode.audio
    
    # Scan through all files and match the format desired
    for file in files:
      if file.format== format:
        self._file_id= int.from_bytes( file.file_id, 
                                       byteorder='big' )
        self._event.clear()
        print( 'Requesting key for track %x file %x' % ( self._track_id, self._file_id ) )
        self._mercury_manager.request_audio_key( self._track_id, 
                                                 self._file_id,
                                                 self._audio_key_callback )
        self._event.wait()
        return True
        
    return False
  
# ----------------------------------- Album metadata and tracks ----------------------------------
class Album:
  def __init__( self, mercury_manager, album_id ):
    self._mercury_manager= mercury_manager
    self._album_id= int.from_bytes( album_id, 
                                    byteorder='big' )
    print( "Album id in HEX:", hex( self._album_id ) )
    self._album= metadata.Album()
    self._event= threading.Event()
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   ALBUM_PATH_TEMPLATE % hex( self._album_id )[ 2: ],
                                   self._info_callback )
    self._event.wait()

  def _info_callback( self, header, parts ):
    self._album.ParseFromString( parts[ 0 ] )
    open( 'album.dat', 'wb' ).write( parts[ 0 ] )
    self._event.set()

# ----------------------------------- Artist metadata and tracks ----------------------------------
class Artist:
  def __init__( self, mercury_manager, artist_id ):
    self._mercury_manager= mercury_manager
    self._artist_id= int.from_bytes( artist_id, 
                                    byteorder='big' )
    print( "Artist id in HEX:", hex( self._artist_id ) )
    self._artist= metadata.Artist()
    self._event= threading.Event()
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   ARTIST_PATH_TEMPLATE % hex( self._artist_id )[ 2: ],
                                   self._info_callback )
    self._event.wait()

  def _info_callback( self, header, parts ):
    self._artist.ParseFromString( parts[ 0 ] )
    open( 'artist.dat', 'wb' ).write( parts[ 0 ] )
    self._event.set()

# ----------------------------------- Playlists for user ----------------------------------
class Playlists:
  def __init__( self, mercury_manager, username ):
    self._mercury_manager= mercury_manager
    self._username= username
    self._event= threading.Event()
    self._playlists= spotify_playlist.SelectedListContent()
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   PLAYLISTS_PATH_TEMPLATE % self._username,
                                   self._info_callback )
    self._event.wait()

  def _info_callback( self, header, parts ):
    self._playlists.ParseFromString( parts[ 0 ] )
    self._event.set()

# ----------------------------------- Playlists for user ----------------------------------
class Playlist:
  def __init__( self, mercury_manager, playlist_id ):
    self._mercury_manager= mercury_manager
    self._playlist_id= playlist_id.replace( ':', '/' )
    self._event= threading.Event()
    self._playlist= spotify_playlist.ListDump()
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   PLAYLIST_CONTENTS_TEMPLATE % self._playlist_id,
                                   self._info_callback )
    self._event.wait()

  def _info_callback( self, header, parts ):
    self._playlist.ParseFromString( parts[ 0 ] )
    self._event.set()
   

# ----------------------------------- User info ----------------------------------
class User:
  def __init__( self, mercury_manager, user ):
    self._mercury_manager= mercury_manager
    self._event= threading.Event()
    self._event.clear()
    self._mercury_manager.execute( REQUEST_TYPE.GET, 
                                   USER_PATH_TEMPLATE % user,
                                   self._info_callback )
    self._event.wait()

  def _info_callback( self, header, parts ):
    print( parts[ 0 ] )
    self._event.set()
   


if __name__ == '__main__':

  if len( sys.argv )!= 3:
    print( 'Usage: spotify.py <username> <password>' )
  else:
    import signal
    manager= None

    def signal_handler(signal, frame):
      if manager:
        manager.terminate()
    
    signal.signal(signal.SIGINT, signal_handler)

    #_track= metadata.Track()
    #_track.ParseFromString( open( 'track.dat', 'rb' ).read() )
    #print( _track )
    #sys.exit()
    
    # AUTHENTICATION_USER_PASS                  - using name/passwd
    # AUTHENTICATION_STORED_SPOTIFY_CREDENTIALS - using reusable_token auth after username/passwd success
    # AUTHENTICATION_SPOTIFY_TOKEN              - ????


    connection = Connection()
    session = Session().connect(connection)
    session.authenticate()
    #print( 'AUTH successfull. Token: ', reusable_token )
    track= None
    episode= None
    manager= MercuryManager( connection )
    while not manager.is_terminated():
      t= input( '>' )
      tt= t.strip().split()
      if len( tt ):
        if tt[ 0 ]== 't':
          track= Track( manager, bytes( tt[ 1 ], 'ascii' ) )
          if not track.load( metadata.AudioFile.Format.Value( 'OGG_VORBIS_320' ) ):
            track= None
          else:
            print( track._track )
        elif tt[ 0 ]== 's':
          if track:
            print( 'Found file matching selected format. fileId= %s' % ( hex(track._file_id), ))
            # Now load some audio data from track
            start_time= time.time()
            aes = pyaes.AESModeOfOperationCTR( track._audio_key.to_bytes( 16, byteorder='big' ), 
                                               pyaes.Counter( int.from_bytes( AUDIO_AESIV, byteorder='big' ) ) )
            f= open( track.track+ '.ogg', 'wb' )
            for i in range( 10000 ):
              chunk_data= track.get_chunk( i )
              if i== 0:
                print( 'File chunk #%d received. Size %d. Header %s' % ( i, len( chunk_data ), track._chunk_header ) )
              else:
                print( 'File chunk #%d received. Size %d' % ( i, len( chunk_data ) ) )
              
              decrypted_chunk= aes.decrypt(chunk_data)
              f.write( decrypted_chunk )
              
              if len( chunk_data )< AUDIO_CHUNK_SIZE:
                print( 'Finished in %d secs' % ( time.time()- start_time ) )
                f.close()
                break
          else:
            print( 'Track with format %d was not found or loaded' % metadata.AudioFile.Format.Value( 'OGG_VORBIS_160' ) )
        elif tt[ 0 ]== 'e':
          episode= Episode( manager, bytes( tt[ 1 ], 'ascii' ) )
          if not episode.load( metadata.AudioFile.Format.Value( 'OGG_VORBIS_96' ) ):
            track= None
          else:
            print( episode._episode )
        elif tt[ 0 ]== 'se':
          if episode:
            print( 'Found file matching selected format. fileId= %s' % ( hex(episode._file_id), ))
            # Now load some audio data from track
            start_time= time.time()
            f= open( '%s_episode.ogg' % episode.episode, 'wb' )
            for i in range( 10000 ):
              chunk_data= episode.get_chunk( i )
              print( 'File chunk #%d received. Size %d' % ( i, len( chunk_data ) ) )
              f.write( chunk_data )
              if len( chunk_data )< AUDIO_CHUNK_SIZE:
                print( 'Finished in %d secs' % ( time.time()- start_time ) )
                f.close()
                break
          else:
            print( 'Track with format %d was not found or loaded' % metadata.AudioFile.Format.Value( 'OGG_VORBIS_160' ) )
        elif tt[ 0 ]== 'a':
          album= Album( manager, ALBUM_GID )
          print( album._album )
        elif tt[ 0 ]== 'r':
          artist= Artist( manager, ARTIST_GID )
        elif tt[ 0 ]== 'p':
          playlists= Playlists( manager, sys.argv[ 1 ] )
          print( playlists._playlists )
        elif tt[ 0 ]== 'auth':
          authToken= AuthToken( manager )
        elif tt[ 0 ]== 'user':
          User( manager, sys.argv[ 1 ] )
        elif tt[ 0 ]== 'pp':
          # playlist format: user:<user>:playlist:<playlist>
          playlist= Playlist( manager, tt[ 1 ] )
          print( playlist._playlist )
        elif tt[ 0 ]== 'q':
          manager.terminate()
