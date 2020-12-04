Attribute VB_Name = "Module1"
Option Compare Database
Option Explicit

Sub test()
    Dim k As New kasaSocket
    
    Debug.Print k.KasaControl("10.0.0.173", "QUERY")
    
End Sub

