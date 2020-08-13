import random
import socket
from zeroconf import Zeroconf, ServiceInfo
from parameter import REMOTE_NAME_DEFAULT
from webserver import HTTPServerController


class Zeroauth:
    MAX_PORT = 65536
    MIN_PORT = 1024

    def __init__(self, deviceName=REMOTE_NAME_DEFAULT):
        self.hostname = socket.gethostname()
 #       try:
  #          self.ip = ip = socket.gethostbyname(self.hostname)
   #     except:
    #        self.hostname += '.local'
     #       self.ip = ip = socket.gethostbyname(self.hostname)

        self.deviceName = deviceName
        self.service = "_spotify-connect._tcp.local."
        self.name = self.deviceName + "._spotify-connect._tcp.local."
        self.port = random.randint(Zeroauth.MIN_PORT, Zeroauth.MAX_PORT)
        self.desc = {"CPath": "/spotzc", "VERSION": "1.0"}

        self.info = ServiceInfo(
            type_=self.service,
            name=self.name,
#            addresses=[socket.inet_aton(self.ip)],
            port=self.port,
            properties=self.desc,
            # server=self.hostname,
        )

        self.zeroconf = Zeroconf()
        self.server = HTTPServerController(self.port)
        # self.register_status_listener = receiver_controller.register_status_listener
        self.register_listener = self.server.register_listener

    def start(self):
        self.server.start_server_thread()
        self.zeroconf.register_service(self.info)

    def stop(self):
        self.zeroconf.unregister_service(self.info)
        self.zeroconf.close()
        self.server.stop_server_thread()
