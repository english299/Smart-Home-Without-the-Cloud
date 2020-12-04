//
// based on
//			https://github.com/branning/hs100
// and
//			https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug
//
// usage
//			node kasasock.js '<ipaddress>', '<command>'
//
// where <command is one of: ON OFF QUERY EMETER
//
// e.g.
//			node kasasock.js '192.168.1.10', 'OFF'
//
//
// you can possibly find all plugs' ip addresses and mac ids using (linux):
//			sudo nmap -Pn -p 9999 --open 10.0.0.0/24 | grep -Ev 'Host|SERVICE|^$|tcp open'
//
//
// raw messages "encrypted" and Base64 encoded
//     const char* payload_on = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu36Lfog=="
//     const char* payload_off = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu3qPeow=="
//     const char* payload_query = "AAAAI9Dw0qHYq9+61/XPtJS20bTAn+yV5o/hh+jK8J7rh+vLtpbr"
//     const char* payload_emeter = "AAAAJNDw0rfav8uu3P7Ev5+92r/LlOaD4o76k/6buYPtmPSYuMXlmA=="
//


var net = require('net');

class kasaSocketNodeJS {

	constructor() {
		this.payload_on_raw = "{\"system\":{\"set_relay_state\":{\"state\":1}}}";
		this.payload_off_raw = "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
		this.payload_query_raw = "{ \"system\":{ \"get_sysinfo\":null } }";
		this.payload_emeter_raw = "{ \"emeter\":{ \"get_realtime\":null } }";
		this.buffer = ''
		this.bufferlen = 0;
		this.retstr="";
	};

	kasaDecrypt() {
		var i;
		var key=171;
		var b=0;

		this.retstr = "";
		this.bufferlen = this.buffer.length;
		for (i = 4; i < this.bufferlen; i++) {
			b=this.buffer[i];
			this.retstr=this.retstr + String.fromCharCode((b ^ key))
			key=b;
			}
		};

	kasaEncrypt(xcmd) {
		var i;
		var key = 171;
		var b=0;

		var x4 = xcmd.length;
		this.buffer=Buffer.alloc(x4+4);

		var x3 = Math.floor(x4 / 256);
		x4 = (x4 - (x3 * 256));
		var x2 = Math.floor(x3 / 256);
		x3 = (x3 - (x2 * 256));
		var x1 = Math.floor(x2 / 256);
		x2 = (x2 - (x1 * 256));

		this.buffer[0] = x1;
		this.buffer[1] = x2;
		this.buffer[2] = x3;
		this.buffer[3] = x4;

		for (i = 4; i < xcmd.length+4; i++) {
			b=xcmd.charCodeAt(i-4) ^ key;
			this.buffer[i]=b;
			key=b;
			}
		//console.log(this.buffer)
		this.bufferlen = xcmd.length + 4;
		};


	kasaControl(ipaddr, cmd) {
		var pcmd=''
		switch (cmd) {
			case "ON":
				pcmd = this.payload_on_raw;
				break;
			case "OFF":
				pcmd = this.payload_off_raw;
				break;
			case "QUERY":
				pcmd = this.payload_query_raw;
				break;
			case "EMETER":
				pcmd = this.payload_emeter_raw;
				break;
		}

		var client = new net.Socket();
		this.kasaEncrypt(pcmd)

		client.connect(9999, ipaddr, function () {
			client.write(k.buffer);
			});

		client.on('end', function () {
			console.log('end');
		});


		client.on('data', function (data) {
			k.buffer = Buffer.from(data);
			client.destroy(); // kill client after server's response
			k.kasaDecrypt();
			console.log(k.retstr);
			});
		}
	}


//=================================================================================================

var k=new kasaSocketNodeJS();
k.kasaControl(process.argv[2], process.argv[3]);
