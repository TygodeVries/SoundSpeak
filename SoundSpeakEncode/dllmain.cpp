// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#pragma unmanaged
#include <Windows.h>
#include <mmeapi.h>
#pragma comment(lib, "winmm")

#include <stdio.h>

#include <queue>

struct sample
{
    int length;
    unsigned char* sample;
};

std::queue<sample> samples;

void wave_cb(HWAVEOUT wave, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WOM_OPEN:
        printf("out open\n");
        break;
    case WOM_CLOSE:
        printf("out close\n");
        break;
    case WOM_DONE:
        WAVEHDR* hdr = (WAVEHDR*)lParam;
        if (hdr->lpData)
        {
            memset(hdr->lpData, 128, 128);
            if (!samples.empty())
            {
                sample t = samples.front();
                samples.pop();
                memcpy(hdr->lpData, t.sample, 128);
                delete t.sample;
            }

            waveOutWrite(wave, (WAVEHDR*)lParam, sizeof(WAVEHDR));
        }
        break;
    }
}
std::queue<unsigned char*> in_samples;
void wave_in_cb(HWAVEIN wave, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WIM_OPEN:
        printf("in open\n");
        break;
    case WIM_CLOSE:
        printf("in close\n");
        break;
    case WIM_DATA:
        WAVEHDR* hdr = (WAVEHDR*)lParam;
        if (hdr->lpData)
        {
            printf("in data\n");
            unsigned char* data = new unsigned char[128];
            memcpy(data, hdr->lpData, 128);
            waveInAddBuffer(wave, hdr, sizeof(WAVEHDR));
            in_samples.push(data);
        }
        break;
    }
}

#include <thread>
ref class SoundHardware;
delegate void soundInput(const char* data, int length);
struct MySoundHardware
{
    void(* callback)(const unsigned char* data, int size);
    std::thread pump;
    ~MySoundHardware()
    {
        running = false;
        pump.join();
    }
    bool running;
    void Run()
    {
        pump = std::thread([&]()
            {
                HWAVEOUT wave;

                WAVEFORMATEX format = { 0 };

                format.cbSize = sizeof(format);
                format.nChannels = 1;
                format.wBitsPerSample = 8;
                format.wFormatTag = WAVE_FORMAT_PCM;
                format.nSamplesPerSec = 11025;
                format.nBlockAlign = 1;
                format.nAvgBytesPerSec = format.nSamplesPerSec * format.wBitsPerSample * format.nChannels / 8;
                MMRESULT result = waveOutOpen(&wave, WAVE_MAPPER, &format, (DWORD_PTR)&wave_cb, 0, CALLBACK_FUNCTION);

                WAVEHDR hdr[4] = { 0 };
                for (int cx = 0; cx < 4; cx++)
                {
                    hdr[cx].dwBufferLength = 128;
                    hdr[cx].lpData = new char[128];
                    waveOutPrepareHeader(wave, &hdr[cx], sizeof(hdr[cx]));
                    waveOutWrite(wave, &hdr[cx], sizeof(hdr[cx]));
                }

                HWAVEIN wave_in;
                result = waveInOpen(&wave_in, WAVE_MAPPER, &format, (DWORD_PTR)&wave_in_cb, 0, CALLBACK_FUNCTION);

                WAVEHDR hdr_in[4] = { 0 };
                for (int cx = 0; cx < 4; cx++)
                {
                    hdr_in[cx].dwBufferLength = 128;
                    hdr_in[cx].lpData = new char[128];
                    waveInPrepareHeader(wave_in, &hdr_in[cx], sizeof(WAVEHDR));
                    waveInAddBuffer(wave_in, &hdr_in[cx], sizeof(WAVEHDR));
                    waveInStart(wave_in);
                }


                while (running)
                {
                    MSG msg;
                    while (PeekMessage(&msg, 0, 0, 0, 1))
                        DispatchMessage(&msg);

                    while (in_samples.size() > 10)
                    {
                        unsigned char sample[10 * 128];
                        for (int cx = 0; cx < 10; cx++)
                        {
                            unsigned char* data = in_samples.front();
                            in_samples.pop();
                            memcpy(sample + cx * 128, data, 128);
                            delete data;
                            if (callback)
                                callback(sample, 1280);
                        }

                    }
                }
            });
    }

    MySoundHardware()
    {
    }
};

#pragma managed
using namespace System;

public ref class SoundHardware
{
    MySoundHardware* inner;
    soundInput^ input;
    void SoundInput(const char* data, int length)
    {
        cli::array<Byte>^ a = gcnew cli::array<Byte>(length);
        for (int cx = 0; cx < length; cx++)
            a[cx] = data[cx];
        if (soundData)
            soundData(a);

    }
public:
    SoundHardware()
    {
        input = gcnew soundInput(this, &SoundHardware::SoundInput);
        inner = new MySoundHardware();
        inner->callback = (void (*)(const unsigned char *, int))
            Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(input).ToPointer();
    }
    ~SoundHardware()
    {
        delete inner;
    }
    void Begin()
    {
        inner->Run();
    }
    void Enqueue(cli::array<System::Byte>^ data)
    {
        for (int offset = 0; offset < data->Length-128; offset += 128)
        {
            unsigned char* d = new unsigned char[data->Length];
            for (int cx = 0; cx < 128; cx++)
                d[cx] = data[offset + cx];
            samples.push({ 128, d });
        }
    }
    void Pump()
    {
    }
    delegate void SoundData(cli::array<Byte>^ data);
    SoundData ^soundData;
};