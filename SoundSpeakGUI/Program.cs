public class Program
{
    static SoundHardware? soundHardware = null;
    public static void Main(string[] args)
    {
        soundHardware = new SoundHardware();
        soundHardware.soundData += (args) => Console.WriteLine("test");
        soundHardware.Begin();
        Random r = new Random();
        for (int cx = 0; cx < 10240; cx++)
        {
            byte[] data = new byte[1024];
            for (int cy = 0; cy < 1024; cy++)
            {
                data[cy] = (byte)r.Next(255);
            }
            soundHardware.Enqueue(data);
        }
        Thread.Sleep(1000);
    }
}