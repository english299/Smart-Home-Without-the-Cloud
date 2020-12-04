Imports System

Module Program
	Sub Main(args As String())
		Dim k As New kasaSocket
		Dim strResult As String

		strResult = k.KasaControl(args(0), args(1))

		Console.WriteLine(strResult)
	End Sub
End Module

