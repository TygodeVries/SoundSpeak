public class Program
{
    static SoundHardware? soundHardware = null;
    public static void Main(string[] args)
    {
        soundHardware = new SoundHardware();
        soundHardware.soundData += (args) => Console.WriteLine("test");
        soundHardware.Begin();
    }
}