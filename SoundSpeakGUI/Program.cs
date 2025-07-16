using SoundSpeakGUI;
using System.Text;

public class Program
{
    public static void Main(string[] args)
    {

        Console.WriteLine("## Welcome to Sound Speak. ##");
        Thread.Sleep(100);
        Console.WriteLine("--  Communication of the future, in 1952. --");

        string[] messages = new string[] {

            "Hello, There!",
            "Hey all!",
            "Good morning!"
        };

        SoundHardware soundHardware = new SoundHardware();
        soundHardware.soundData += (byte[] args) => OnData(args);
        soundHardware.Begin();

        string inputMessage = "";
        DrawUI(messages, inputMessage);
        while (true)
        {
            if (Console.KeyAvailable)
            {
                ConsoleKeyInfo key = Console.ReadKey();

                Console.SetCursorPosition(0, 0);
                if (key.Key == ConsoleKey.Enter)
                {
                    inputMessage = "";
                }
                else if (key.Key == ConsoleKey.Backspace)
                {
                    inputMessage = inputMessage.Remove(inputMessage.Length - 1);
                }
                else
                {
                    inputMessage += key.KeyChar;
                }

                DrawUI(messages, inputMessage);
            }
        }
    }

    public static void OnData(byte[] audioData)
    {

    }


    public static void DrawUI(string[] messages, string inputMessage)
    {
        Console.Clear();

        Console.WriteLine("- Recent Messages -");
        foreach (string message in messages)
        {
            Console.WriteLine($":{message}");
        }

        Console.WriteLine();
        Console.WriteLine("- Incoming Message -");
        Console.WriteLine("Blah bl...");

        Console.WriteLine();
        Console.WriteLine();
        Console.WriteLine("> " + inputMessage);
        Console.CursorVisible = false;
    }
}