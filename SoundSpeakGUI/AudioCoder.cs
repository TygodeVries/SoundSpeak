using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SoundSpeakGUI
{
    public class AudioCoder
    {
        public byte Decode(byte[] data)
        {
            return 0;
        }

        public byte[] Encode(byte data)
        {

            byte[] bytes = new byte[1024];

            for(int i = 0; i < 1024; i++)
            {
                bytes[i] = (byte) Math.Floor(Math.Cos(i / (1024 / 4)) * 255);
            }

            return bytes;
        }
    }
}
