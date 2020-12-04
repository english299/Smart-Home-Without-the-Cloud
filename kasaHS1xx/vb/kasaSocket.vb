
Imports System.Net
Imports System.Text
Imports System.Net.Sockets
Imports System.Threading

Public Class kasaSocket

    '#
    '# based on
    '#			https://github.com/branning/hs100
    '# And
    '#			https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug
    '#
    '# usage
    '#			<program.exe> '<ipaddress>', '<command>'
    '#
    '# where <command Is one of: ON OFF QUERY EMETER
    '#
    '# e.g.
    '#			kasaHS100.exe '192.168.1.10', 'OFF'
    '#
    '#
    '# you can possibly find all plugs' ip addresses and mac ids using (linux):
    '#			sudo nmap -Pn -p 9999 --open 10.0.0.0/24 | grep -Ev 'Host|SERVICE|^$|tcp open'
    '#


    '# raw messages "encrypted" And Base64 encoded
    '#     Public Shared payload_on = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu36Lfog=="
    '#     Public Shared payload_off = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu3qPeow=="
    '#     Public Shared payload_query = "AAAAI9Dw0qHYq9+61/XPtJS20bTAn+yV5o/hh+jK8J7rh+vLtpbr"
    '#     Public Shared payload_emeter = "AAAAJNDw0rfav8uu3P7Ev5+92r/LlOaD4o76k/6buYPtmPSYuMXlmA=="


    Public buffer(1024) As Byte
    Public inmsglen As Integer

    Public Shared payload_on_raw As String = "{""system"":{""set_relay_state"":{""state"":1}}}"         '{"system":{"set_relay_state":{"state":1}}}
    Public Shared payload_off_raw As String = "{""system"":{""set_relay_state"":{""state"":0}}}"        '{"system":{"set_relay_state":{"state":1}}}
    Public Shared payload_query_raw As String = "{ ""system"":{ ""get_sysinfo"":null } }"               '{ "system":{ "get_sysinfo":null } }
    Public Shared payload_emeter_raw As String = "{ ""emeter"":{ ""get_realtime"":null } }"             '{ "emeter":{ "get_realtime":null } }


    Function KasaDecrypt() As String
        Dim key As Byte = 171 'initial key
        Dim q As Byte
        Dim lstr_S As String = ""
        Dim lint_I As Integer
        Dim msgsize As Int32

        msgsize = 0

        msgsize += buffer(0)
        msgsize *= 256
        msgsize += buffer(1)
        msgsize *= 256
        msgsize += buffer(2)
        msgsize *= 256
        msgsize += buffer(3)


        For lint_I = 4 To msgsize + 4
            q = buffer(lint_I)
            buffer(lint_I) = q Xor key
            key = q
        Next lint_I
        lstr_S = Encoding.ASCII.GetString(buffer, 4, msgsize)
        Return lstr_S
    End Function


    Function KasaEncrypt(pstr_Msg As String) As Byte()
        Dim key As Byte = 171
        Dim lstr_S As String = ""
        Dim lint_I As Integer
        Dim msg As Byte()

        lstr_S = Chr(0) & Chr(0) & Chr(0) & Chr(pstr_Msg.Length) & pstr_Msg

        msg = Encoding.ASCII.GetBytes(lstr_S)

        For lint_I = 4 To msg.Length - 1
            msg(lint_I) = key Xor msg(lint_I)
            key = msg(lint_I)
        Next lint_I

        Return msg

    End Function


    Sub KasaSetOn(pstr_IP As String)
        Call KasaControl(pstr_IP, "ON")
    End Sub

    Sub KasaSetOff(pstr_IP As String)
        Call KasaControl(pstr_IP, "OFF")
    End Sub

    Function KasaStatus(pstr_IP As String) As String
        Return KasaControl(pstr_IP, "QUERY")
    End Function

    Function KasaMeter(pstr_IP As String) As String
        Return KasaControl(pstr_IP, "EMETER")
    End Function

    Function KasaControl(ipaddress As String, pstr_Command As String) As String

        Dim nStream As NetworkStream
        Dim tClient As New System.Net.Sockets.TcpClient()
        Dim msg As Byte()
        Dim lstr_S As String = ""


        tClient.Connect(ipaddress, 9999)
        nStream = tClient.GetStream()

        If nStream.CanWrite And nStream.CanRead Then
            Select Case pstr_Command
                Case "ON"
                    msg = KasaEncrypt(payload_on_raw)
                Case "OFF"
                    msg = KasaEncrypt(payload_off_raw)
                Case "QUERY"
                    msg = KasaEncrypt(payload_query_raw)
                Case "EMETER"
                    msg = KasaEncrypt(payload_emeter_raw)
            End Select
            nStream.Write(msg, 0, msg.Length)
            inmsglen = nStream.Read(buffer, 0, buffer.Length)
            lstr_S = KasaDecrypt()
        Else
            If Not nStream.CanRead Then
                Console.WriteLine("cannot not write data to this stream")
                tClient.Close()
            Else
                If Not nStream.CanWrite Then
                    Console.WriteLine("cannot read data from this stream")
                    tClient.Close()
                End If
            End If
        End If
        ' pause so user can view the console output
        tClient.Close()
        Return lstr_S
    End Function



End Class



