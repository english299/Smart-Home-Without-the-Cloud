using System;

namespace kasasockcsharp
	{
	class Program
		{
		static void Main(string[] args)
			{
			//Console.WriteLine("Hello World!");
			kasasock k = new kasasock();
			

			Console.WriteLine(k.KasaControl(args[0], args[1]));

			}
		}
	}

