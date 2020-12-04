#pragma once

#include <netdb.h> /* struct hostent, gethostbyname */

/*
#
# based on
#			https://github.com/branning/hs100
# and
#			https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug
#
# usage
#			<program> '<ipaddress>', '<command>'
#
# where <command is one of: ON OFF QUERY EMETER
#
# e.g.
#			kasaHS100 '192.168.1.10', 'OFF'
#
#
# you can possibly find all plugs' ip addresses and mac ids using (linux):
#			sudo nmap -Pn -p 9999 --open 10.0.0.0/24 | grep -Ev 'Host|SERVICE|^$|tcp open'
#


# raw messages "encrypted" and Base64 encoded
#     const char* payload_on = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu36Lfog=="
#     const char* payload_off = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu3qPeow=="
#     const char* payload_query = "AAAAI9Dw0qHYq9+61/XPtJS20bTAn+yV5o/hh+jK8J7rh+vLtpbr"
#     const char* payload_emeter = "AAAAJNDw0rfav8uu3P7Ev5+92r/LlOaD4o76k/6buYPtmPSYuMXlmA=="
*/



class kasaSocket
	{
	public:
		const char* payload_on_raw = "{\"system\":{\"set_relay_state\":{\"state\":1}}}";
		const char* payload_off_raw = "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
		const char* payload_query_raw = "{ \"system\":{ \"get_sysinfo\":null } }";
		const char* payload_emeter_raw = "{ \"emeter\":{ \"get_realtime\":null } }";

		uint16_t port = 9999;

		uint8_t buffer[1024];

		int msglen = 0;

		char* response = (char*)&buffer[4];

		int kasaEncrypt(const char* msg);
		int kasaDecrypt(const char* p);
		int kasaControl(const char* ipaddress, const char* cmd);
		int kasaSetOn(const char* ipaddress);
		int kasaSetOff(const char* ipaddress);
		int kasaQuery(const char* ipaddress);
		int kasaMeter(const char* ipaddress);
		int kasaConnect(int* sck, const char* ipaddress, hostent* server, struct sockaddr_in* inaddr);

	};


