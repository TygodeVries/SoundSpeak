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
        printf("out sample\n");
        WAVEHDR* hdr = (WAVEHDR*)lParam;
        memset(hdr->lpData, 128, 128);
        if (!samples.empty())
        {
            sample t = samples.front();
            samples.pop();
            memcpy(hdr->lpData, t.sample, 128);
            delete t.sample;
        }

        waveOutWrite(wave, (WAVEHDR*)lParam, sizeof(WAVEHDR));
        break;
    }
}

struct MySoundHardware
{
    void(* callback)(const char* data, int size);
    MySoundHardware(void (*callback)(const char* data, int size)):callback(callback)
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
    }
};

#pragma managed
using namespace System;

public ref class SoundHardware
{
    MySoundHardware* inner;
    delegate void soundInput(const char* data, int length);
    soundInput ^input;
    void SoundInput(const char* data, int length)
    {
    }
public:
    SoundHardware()
    {
        input += gcnew soundInput(this, &SoundHardware::SoundInput);
        inner = new MySoundHardware(nullptr);
    }
    ~SoundHardware()
    {
        delete inner;
    }
    void Begin()
    {
        if (soundData)
            soundData(nullptr);
    }
    void Enqueue(cli::array<System::Byte>^ data)
    {
        for (int offset = 0; offset < data->Length; offset += 128)
        {
            unsigned char* d = new unsigned char[data->Length];
            for (int cx = 0; cx < 128; cx++)
                d[cx] = data[cx];
            samples.push({ 128, d });
        }
    }
    delegate void SoundData(cli::array<Byte>^ data);
    SoundData ^soundData;
};