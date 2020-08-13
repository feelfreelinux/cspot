from zeroauth import Zeroauth
from time import sleep
za = Zeroauth()
za.start()

while True:
    sleep(1)
