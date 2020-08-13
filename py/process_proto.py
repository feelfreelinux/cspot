

import os,         \
       subprocess

class EProtobufferIsMissing(Exception):
  pass

# Helper function to generate protper PROTOBUF Python stubs
def process_proto( src_path,
                   dest_path ):
  # Check if protoc installed
  protoc_version= subprocess.check_output( [ 'protoc', '--version' ] )
  if protoc_version.split()[ 0 ].decode( 'ascii' )!= 'libprotoc':
    raise EProtobufferIsMissing( 'protoc command was not found. Make sure to install protobuffer suite: https://developers.google.com/protocol-buffers/' )
    
  for file in os.listdir( src_path ):
    print( 'Processing', file )
    if os.path.isfile( os.path.join( src_path, file ) ):
      # run protoc to update all PROTO declarations
      print( subprocess.check_output( [ 'protoc', '-I='+ src_path, '--python_out='+ dest_path, os.path.join( src_path, file ) ] ) )

if __name__ == '__main__':
  process_proto( 'protocol/proto', 'protocol/impl' )    
