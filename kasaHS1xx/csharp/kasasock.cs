using System;
using System.Collections.Generic;
using System.Text;
using System.Net.Sockets;

/*
#
# based on
#			https://github.com/branning/hs100
# and
#			https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug
#
# usage
#			<program.exe> '<ipaddress>', '<command>'
#
# where <command is one of: ON OFF QUERY EMETER
#
# e.g.
#			kasaHS100.exe '192.168.1.10', 'OFF'
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


namespace kasasockcsharp
	{
	class kasasock
		{
		//Int32 port = 13000;
		//string server = "10.0.0.173";
		//string cmd = "ON";

		string payload_on_raw = "{\"system\":{\"set_relay_state\":{\"state\":1}}}";
		string payload_off_raw = "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
		string payload_query_raw = "{ \"system\":{ \"get_sysinfo\":null } }";
		string payload_emeter_raw = "{ \"emeter\":{ \"get_realtime\":null } }";


		string KasaDecrypt(byte[] buf, Int32 buflen)
			{
			byte key = 171;
			byte b;
			for (int i=4; i<buflen;i++)
				{
				b = buf[i];
				buf[i] = (byte)(buf[i] ^ key);
				key = b;
				}
			return Encoding.ASCII.GetString(buf, 4, buflen - 4);
			}

		byte[] KasaEncrypt(string message)
			{
			Int32 l= message.Length;
			Int32 x4 = l;
			Int32 x3=x4/256;
			x4 -= (256 * x3);
			Int32 x2 = x3 / 256;
			x3 -= (256 * x2);
			Int32 x1 = x2 / 256;
			x2 -= (256 * x1);

			Byte key = 171;
			byte b;


			var builder = new StringBuilder();
			builder.Append(((char)x1).ToString());
			builder.Append(((char)x2).ToString());
			builder.Append(((char)x3).ToString());
			builder.Append(((char)x4).ToString());
			builder.Append(message);

			message = builder.ToString();


			byte[] buff= Encoding.ASCII.GetBytes(message);

			for(l=4;l<buff.Length;l++)
				{
				b = (byte)(key ^ buff[l]);
				buff[l] = b;
				key = b;
				}
			return buff;
			}
		public string KasaControl(string server, string cmd)
			{
			string pcmd="";

			byte[] inbuff = new byte[1024];

			switch(cmd)
				{
				case "ON":
					pcmd = payload_on_raw;
					break;
				case "OFF":
					pcmd = payload_off_raw;
					break;
				case "QUERY":
					pcmd = payload_query_raw;
					break;
				case "EMETER":
					pcmd = payload_emeter_raw;
					break;
				}


			TcpClient client = new TcpClient(server, 9999);

			NetworkStream stream = client.GetStream();

			stream.Write(KasaEncrypt(pcmd));

			Int32 readlen=stream.Read(inbuff, 0, 1024);


			return KasaDecrypt(inbuff, readlen);

			}
		}
	}

