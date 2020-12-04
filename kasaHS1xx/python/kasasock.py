import sys
import socket

#
# based on
#                       https://github.com/branning/hs100
# and
#                       https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug
#
# usage
#                       python kasasock.py '<ipaddress>', '<command>'
#
# where <command is one of: ON OFF QUERY EMETER
#
# e.g.
#                       python kasasock.py '192.168.1.10', 'OFF'
#
#
# you can possibly find all plugs' ip addresses and mac ids using (linux):
#                       sudo nmap -Pn -p 9999 --open 10.0.0.0/24 | grep -Ev 'Host|SERVICE|^$|tcp open'
#
#
# raw messages "encrypted" and Base64 encoded
#     payload_on = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu36Lfog=="
#     payload_off = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu3qPeow=="
#     payload_query = "AAAAI9Dw0qHYq9+61/XPtJS20bTAn+yV5o/hh+jK8J7rh+vLtpbr"
#     payload_emeter = "AAAAJNDw0rfav8uu3P7Ev5+92r/LlOaD4o76k/6buYPtmPSYuMXlmA=="

class KasaSocket:

    def __init__(self):
        self.port=9999
        self.payload_on_raw = '{"system":{"set_relay_state":{"state":1}}}'
        self.payload_off_raw = '{"system":{"set_relay_state":{"state":0}}}'
        self.payload_query_raw = '{ "system":{ "get_sysinfo":null } }'
                #HS100 does not support this, HS110 does
        self.payload_emeter_raw = '{ "emeter":{ "get_realtime":null } }'

    def kasaDecrypt(self,mymsg):
        k=171
        mymsg=bytearray(mymsg[4:]) #force it to bytearray and skip length bytes

        for x in range(len(mymsg)): #decrypt
            b=mymsg[x]
            mymsg[x]=k^b
            k=b

        return mymsg.decode('ascii') #return string



    def kasaEncrypt(self,pstr):
        #
        # comes in as string leaves as encrypted bytearray
        #

        k=171 #initial encryption key
        mymsg=bytearray(pstr.encode('ascii'))

        x4=len(mymsg) #clunky transform int32
        x3=x4//256
        x4=x4-256*x3
        x2=x3//256
        x3=x3-256*x2
        x1=x2//256
        x2=x2-256*x1

        for x in range(len(mymsg)): #encrypt
            b=k^mymsg[x]
            mymsg[x]=b
            k=b

        mymsg.insert(0,x4) #more clunkiness insert the length onto start of array
        mymsg.insert(0,x3)
        mymsg.insert(0,x2)
        mymsg.insert(0,x1)

        return mymsg

    def KasaSetOn(self, ipaddress):
        return self.KasaControl(ipaddress,'ON')

    def KasaSetOff(self, ipaddress):
        return self.KasaControl(ipaddress,'OFF')

    def KasaStatus(self, ipaddress):
        return self.KasaControl(ipaddress,'QUERY')

    def KasaMeter(self, ipaddress):
        return self.KasaControl(ipaddress,'EMETER')


    def KasaControl(self, ipaddress, cmd):
        if cmd=='ON':
            msg=self.payload_on_raw
        if cmd=='OFF':
            msg=self.payload_off_raw
        if cmd=='QUERY':
            msg=self.payload_query_raw
        if cmd=='EMETER':
            msg=self.payload_emeter_raw

        s = socket.socket()
        s.connect((ipaddress,self.port))

        ba=self.kasaEncrypt(msg)

        s.send(ba)

        reply=s.recv(1024)
        msg=self.kasaDecrypt(reply)
        return msg


#===========================================================================================================

mycmd=sys.argv[2]
myip=sys.argv[1]

k=KasaSocket()

print(k.KasaControl(myip,mycmd))


# -- or explicit commands --
# k=KasaSocket()
# print(k.KasaSetOn(myip))
# print(k.KasaSetOff(myip))
# print(k.KasaStatus(myip)
# print(k.KasaMeter(myip)


