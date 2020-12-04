VERSION 1.0 CLASS
BEGIN
  MultiUse = -1  'True
END
Attribute VB_Name = "kasaSocket"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = False
Attribute VB_Exposed = False
Option Compare Database
Option Explicit

'
' Windows Sockets code is based on Bogey's TestWinsock (Stackoverflow)
'
'
'#
'# based on
'#           https://github.com/branning/hs100
'# and
'#           https://github.com/ggeorgovassilis/linuxscripts/tree/master/tp-link-hs100-smartplug
'#
'# usage
'#           as per Module1
'#
'# where command is one of: ON OFF QUERY EMETER
'#
'#
'#
'# you can possibly find all plugs' ip addresses and mac ids using (linux):
'#           sudo nmap -Pn -p 9999 --open 10.0.0.0/24 | grep -Ev 'Host|SERVICE|^$|tcp open'
'#


'# raw messages "encrypted" and Base64 encoded
'#     payload_on = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu36Lfog=="
'#     payload_off = "AAAAKtDygfiL/5r31e+UtsWg1Iv5nPCR6LfEsNGlwOLYo4HyhueT9tTu3qPeow=="
'#     payload_query = "AAAAI9Dw0qHYq9+61/XPtJS20bTAn+yV5o/hh+jK8J7rh+vLtpbr"
'#     payload_emeter = "AAAAJNDw0rfav8uu3P7Ev5+92r/LlOaD4o76k/6buYPtmPSYuMXlmA=="



    Public payload_on_raw As String
    Public payload_off_raw As String
    Public payload_query_raw As String
    Public payload_emeter_raw As String


    Public port As Integer
    
    Dim buffer(0 To 1023) As Byte
    Dim int_BufLen As Integer
    
' Constants ----------------------------------------------------------
Const INVALID_SOCKET = -1
Const WSADESCRIPTION_LEN = 256
Const SOCKET_ERROR = -1

' Typ definitions ----------------------------------------------------

Private Type wsaData
    wVersion As Integer
    wHighVersion As Integer
    szDescription(0 To WSADESCRIPTION_LEN) As Byte
    szSystemStatus(0 To WSADESCRIPTION_LEN) As Byte
    iMaxSockets As Integer
    iMaxUdpDg As Integer
    lpVendorInfo As Long
End Type

Private Type ADDRINFO
    ai_flags As Long
    ai_family As Long
    ai_socktype As Long
    ai_protocol As Long
    ai_addrlen As Long
    ai_canonName As LongPtr 'strptr
    ai_addr As LongPtr 'p sockaddr
    ai_next As LongPtr 'p addrinfo
End Type


' Enums ---------------------------------------------------------------

Enum AF
AF_UNSPEC = 0
AF_INET = 2
AF_IPX = 6
AF_APPLETALK = 16
AF_NETBIOS = 17
AF_INET6 = 23
AF_IRDA = 26
AF_BTH = 32
End Enum

Enum sock_type
SOCK_STREAM = 1
SOCK_DGRAM = 2
SOCK_RAW = 3
SOCK_RDM = 4
SOCK_SEQPACKET = 5
End Enum

' External functions --------------------------------------------------

'Public Declare Function WSAStartup Lib "ws2_32.dll" (ByVal wVersionRequested As Integer, ByRef data As WSADATA) As Long

Private Declare Function connect Lib "ws2_32.dll" (ByVal socket As Long, ByVal sockaddr As Long, ByVal namelen As Long) As Long
Private Declare Sub WSACleanup Lib "ws2_32.dll" ()
Private Declare PtrSafe Function GetAddrInfo Lib "ws2_32.dll" Alias "getaddrinfo" (ByVal NodeName As String, ByVal ServName As String, ByVal lpHints As LongPtr, lpResult As LongPtr) As Long
Private Declare Function ws_socket Lib "ws2_32.dll" Alias "socket" (ByVal AF As Long, ByVal stype As Long, ByVal Protocol As Long) As Long
Private Declare Function closesocket Lib "ws2_32.dll" (ByVal socket As Long) As Long
Private Declare PtrSafe Sub CopyMemory Lib "kernel32" Alias "RtlMoveMemory" (Destination As Any, Source As Any, ByVal length As Long)

Private Declare Function send Lib "ws2_32.dll" (ByVal s As Long, ByRef buf As Any, ByVal BufLen As Long, ByVal flags As Long) As Long
Private Declare Function recv Lib "ws2_32.dll" (ByVal s As Long, ByRef buf As Any, ByVal BufLen As Long, ByVal flags As Long) As Long

Private Declare Function SendWithPtr Lib "ws2_32.dll" Alias "send" (ByVal s As Long, ByVal bufPtr As Long, ByVal BufLen As Long, ByVal flags As Long) As Long
Private Declare PtrSafe Function WSAGetLastError Lib "ws2_32.dll" () As Long
Private Declare Function VarPtrArray Lib "VBE7" Alias "VarPtr" (var() As Any) As Long
Private Declare Function WSAStartup Lib "ws2_32.dll" (ByVal wVersionRequested As Long, ByRef data As wsaData) As Long

Sub SendAndReceive(pstr_Server As String)
    Dim m_wsaData As wsaData
    Dim m_RetVal As Integer
    Dim m_Hints As ADDRINFO
    Dim m_ConnSocket As Long: m_ConnSocket = INVALID_SOCKET
    Dim Server As String
    Dim port As String
    Dim pAddrInfo As LongPtr
    Dim RetVal As Long
    Dim lastError As Long

    RetVal = WSAStartup(&H202, m_wsaData)
    If (RetVal <> 0) Then
        LogError "WSAStartup failed with error " & RetVal, WSAGetLastError()
        Call WSACleanup
        Exit Sub
    End If

    m_Hints.ai_family = AF.AF_UNSPEC
    m_Hints.ai_socktype = sock_type.SOCK_STREAM
    Server = "10.0.0.173"
    port = "9999"

    RetVal = GetAddrInfo(Server, port, VarPtr(m_Hints), pAddrInfo)
    If (RetVal <> 0) Then
        LogError "Cannot resolve address " & Server & " and port " & port & ", error " & RetVal, WSAGetLastError()
        Call WSACleanup
        Exit Sub
    End If

    m_Hints.ai_next = pAddrInfo
    Dim connected As Boolean: connected = False
    Do While m_Hints.ai_next > 0
        CopyMemory m_Hints, ByVal m_Hints.ai_next, LenB(m_Hints)

        m_ConnSocket = ws_socket(m_Hints.ai_family, m_Hints.ai_socktype, m_Hints.ai_protocol)

        If (m_ConnSocket = INVALID_SOCKET) Then
            LogError "Error opening socket, error " & RetVal
        Else
            Dim connectionResult As Long

            connectionResult = connect(m_ConnSocket, m_Hints.ai_addr, m_Hints.ai_addrlen)

            If connectionResult <> SOCKET_ERROR Then
                connected = True
                Exit Do
            End If

            LogError "connect() to socket failed"
            closesocket (m_ConnSocket)
        End If
    Loop

    If Not connected Then
        LogError "Fatal error: unable to connect to the server", WSAGetLastError()
        Call WSACleanup
        Exit Sub
    End If

    RetVal = send(m_ConnSocket, buffer(0), int_BufLen, 0)

    If RetVal = SOCKET_ERROR Then
        LogError "send() failed", WSAGetLastError()
        Call WSACleanup
        Exit Sub
    Else
        RetVal = recv(m_ConnSocket, buffer(0), 1024, 0)
        If RetVal > 0 Then
            int_BufLen = RetVal
        Else
            int_BufLen = -1
        End If
    End If

    RetVal = closesocket(m_ConnSocket)
    If RetVal <> 0 Then
    LogError "closesocket() failed", WSAGetLastError()
    Call WSACleanup
    Else
        'Debug.Print "closed socket"
    End If
End Sub

Public Function MAKEWORD(Lo As Byte, Hi As Byte) As Integer
MAKEWORD = Lo + Hi * 256& Or 32768 * (Hi > 127)
End Function

Private Sub LogError(msg As String, Optional ErrorCode As Long = -1)
    If ErrorCode > -1 Then
        msg = msg & " (error code " & ErrorCode & ")"
    End If

    Debug.Print msg
End Sub


Private Sub Class_Initialize()
    port = 9999
    
    payload_on_raw = "{""system"":{""set_relay_state"":{""state"":1}}}"
    payload_off_raw = "{""system"":{""set_relay_state"":{""state"":0}}}"
    payload_query_raw = "{ ""system"":{ ""get_sysinfo"":null } }"
    payload_emeter_raw = "{ ""emeter"":{ ""get_realtime"":null } }"
    
End Sub



Public Function KasaControl(ipaddress As String, cmd As String) As String
    Dim msg As String
    
    KasaControl = ""
    Select Case cmd
        Case "ON"
            msg = payload_on_raw
        Case "OFF"
            msg = payload_off_raw
        Case "QUERY"
            msg = payload_query_raw
        Case "EMETER"
            msg = payload_emeter_raw
    End Select
    
    Call KasaEncrypt(msg)
    
    Call SendAndReceive(ipaddress)
    
    KasaControl = KasaDecrypt
    
End Function

Public Sub KasaEncrypt(pstr_Message As String)
    Dim key As Byte
    Dim lint_I As Integer
    Dim lint_Z4 As Integer
    Dim lint_Z3 As Integer
    int_BufLen = Len(pstr_Message)
    lint_Z4 = int_BufLen
    lint_Z3 = lint_Z4 \ 256
    lint_Z4 = lint_Z4 - (lint_Z3 * 256)
    
    For lint_I = 1 To int_BufLen
        buffer(lint_I + 3) = Asc(Mid(pstr_Message, lint_I, 1))
    Next lint_I
    
    buffer(3) = lint_Z4
    buffer(2) = lint_Z3
    buffer(1) = 0
    buffer(0) = 0
    
    int_BufLen = int_BufLen + 4
    
    key = 171

    
    For lint_I = 4 To int_BufLen - 1
        buffer(lint_I) = key Xor buffer(lint_I)
        key = buffer(lint_I)
    Next lint_I

End Sub


Function KasaDecrypt() As String
    Dim lint_I As Integer
    Dim lbyt_B As Byte
    Dim key As Byte
    
    KasaDecrypt = ""
    
    key = 171
    
    For lint_I = 4 To int_BufLen - 1
        KasaDecrypt = KasaDecrypt & Chr$(key Xor buffer(lint_I))
        key = buffer(lint_I)
    Next lint_I
    
End Function

