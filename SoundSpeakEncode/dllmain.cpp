// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#pragma unmanaged
#include <Windows.h>
#include <mmeapi.h>
#pragma comment(lib, "winmm")
#define _USE_MATH_DEFINES
#include <math.h>

#include <stdio.h>

struct complex
{
    float r, i;
    complex(float r, float i) :r(r), i(i) {}
    complex(float r) :r(r), i(0) {}
    complex() {}
    complex(const complex& x)
    {
        r = x.r;
        i = x.i;
    }
    complex& operator += (const complex& x)
    {
        r += x.r;
        i += x.i;
        return *this;
    }
    complex operator +(const complex& x)
    {
        return { r + x.r, i + x.i };
    }
    complex operator -(const complex& x)
    {
        return { r - x.r, i - x.i };
    }
    complex operator *(const complex& x)
    {
        return { r * x.r - i * x.i, r * x.i + i * x.r };
    }
    complex operator /(const float& x)
    {
        return { r / x, i / x };
    }
    float mag()
    {
        return sqrtf(r * r + i * i);
    }
};

complex exp(const complex& x)
{
    const float e = expf(x.r);
    return { e * cosf(x.i), e * sinf(x.i) };
}
#define PI 3.14159265f
template <int N>
void FFT(complex input[N], complex output[N])
{
    for (int cx = 0; cx < N; cx++)
    {
        complex q = 0;
        for (int cy = 0; cy < N; cy++)
            q += exp({ 0, 2.f * PI * cx * cy / N }) * input[cy];
        output[cx] = q / N;
    }
}
template <int N>
void iFFT(complex input[N], complex output[N])
{
    for (int cx = 0; cx < N; cx++)
    {
        complex q = 0;
        for (int cy = 0; cy < N; cy++)
            q += exp({ 0, -2.f * PI * cx * cy / N }) * input[cy];
        output[cx] = q / N;
    }
}


#include <queue>

struct sample
{
    int length;
    unsigned char* sample;
};

struct section_t
{
    CRITICAL_SECTION section;
    section_t()
    {
        InitializeCriticalSection(&section);
    }
    void lock()
    {
        EnterCriticalSection(&section);
    }
    void unlock()
    {
        LeaveCriticalSection(&section);
    }
} samples_lock;

std::queue<sample> samples;

void wave_cb(HWAVEOUT wave, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WOM_OPEN:
        printf("Soundcard output opened\n");
        break;
    case WOM_CLOSE:
        printf("Soundcard output closed\n");
        break;
    case WOM_DONE:
        WAVEHDR* hdr = (WAVEHDR*)lParam;
        memset(hdr->lpData, 128, 128);
        samples_lock.lock();
        if (!samples.empty())
        {
            sample t = samples.front();
            samples.pop();
            memcpy(hdr->lpData, t.sample, 128);
            delete t.sample;
        }
        samples_lock.unlock();
        waveOutWrite(wave, hdr, sizeof(WAVEHDR));
        break;
    }
}
std::queue<unsigned char*> in_samples;
void wave_in_cb(HWAVEIN wave, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WIM_OPEN:
        printf("Soundcard input opened\n");
        break;
    case WIM_CLOSE:
        printf("Soundcard input closed\n");
        break;
    case WIM_DATA:
        WAVEHDR* hdr = (WAVEHDR*)lParam;
        if (hdr->lpData)
        {
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
    HWND wnd;
    HDC dc;
    HDC memdc;
    HBITMAP bmp;
    unsigned long* pixels;
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
                running = true;
                int line = 0;
                RECT client = { 0 };
                client.right = client.bottom = 512;
                AdjustWindowRect(&client, WS_CAPTION, FALSE);
                client.right -= client.left;
                client.bottom -= client.top;
                wnd = CreateWindowA("BUTTON", "Fourier", WS_VISIBLE, 0, 0, client.right, client.bottom, 0, 0, 0, 0);
                dc = GetDC(wnd);
                memdc = CreateCompatibleDC(dc);
                pixels = new unsigned long[512 * 512];
                bmp = CreateBitmap(512, 512, 1, 32, pixels);
                SelectObject(memdc, bmp);
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
                Sleep(1000);
                HWAVEIN wave_in;
                result = waveInOpen(&wave_in, WAVE_MAPPER, &format, (DWORD_PTR)&wave_in_cb, 0, CALLBACK_FUNCTION);
                WAVEHDR hdr_in[4] = { 0 };
                for (int cx = 0; cx < 4; cx++)
                {
                    hdr_in[cx].dwBufferLength = 128;
                    hdr_in[cx].lpData = new char[1280];
                    waveInPrepareHeader(wave_in, &hdr_in[cx], sizeof(WAVEHDR));
                    waveInAddBuffer(wave_in, &hdr_in[cx], sizeof(WAVEHDR));
                    waveInStart(wave_in);
                }


                while (running)
                {
                    MSG msg;
                    while (PeekMessage(&msg, 0, 0, 0, 1))
                        DispatchMessage(&msg);
                    BitBlt(dc, 0, 0, 512, 512, memdc, 0, 0, SRCCOPY);
                    if (in_samples.size() > 10)
                        while (in_samples.size() > 10)
                        {
                            unsigned char sample[1024];
                            for (int cx = 0; cx < 8; cx++)
                            {
                                unsigned char* data = in_samples.front();
                                in_samples.pop();
                                memcpy(sample + cx * 128, data, 128);
                                delete data;
                            }
                            complex in[1024], out[1024];

                            for (int cx = 0; cx < 1024; cx++)
                                in[cx] = complex((float)sample[cx]);
                            FFT<1024>(in, out);
                            float m = 0.5;// out[1].mag();
                            for (int cx = 1; cx < 512; cx++)
                                m = max(m, out[cx].mag());
                            unsigned char fft_sample[512];
                            for (int cx = 0; cx < 512; cx++)
                            {
                                unsigned long c = 255 * out[cx].mag() / m;
                                fft_sample[cx] = c;
                                c |= c << 8;
                                c |= c << 8;
                                pixels[cx + 512 * (line & 511)] = c;
                            }
                            line++;
                            SetBitmapBits(bmp, 512 * 512 * 4, pixels);
                            if (callback)
                                callback(fft_sample, 512);

                        }
                    else
                        SleepEx(100, TRUE);
                }
                waveInStop(wave_in);
                waveOutReset(wave);
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
    int time = 0;
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
    /// <summary>
    /// Enqueue data to be sent to the soundcard
    /// 
    /// </summary>
    /// <param name="data">This must be a 512 entry byte array, each entry specifies the amplitude of the frequency for that index</param>
    void Enqueue(cli::array<System::Byte>^ data)
    {
        if (data->Length != 512)
        {
            printf("Array length must be 512\n");
            return;
        }
        float sample[2048];
        for (int cy = 0; cy < 2048; cy++)
        {
            float window = (1.f - cosf(2.f * M_PI * cy / 2048.f)) / 2;
            sample[cy] = 127;
            for (int cx = 0; cx < 512; cx++)
            {
                sample[cy] += data[cx] * sinf((float)time / 1024 * cx) * window;
            }
            sample[cy] = max(0, min(255, sample[cy]));
            time++;
        }
        for (int offset = 0; offset < 2048; offset += 128)
        {
            unsigned char* d = new unsigned char[data->Length];
            for (int cx = 0; cx < 128; cx++)
                d[cx] = sample[offset + cx];
            samples_lock.lock();
            samples.push({ 128, d });
            samples_lock.unlock();
        }
    }
    delegate void SoundData(cli::array<Byte>^ data);
    /// <summary>
    /// Register a handler to receive samples from the sound card.
    /// A byte array is given with amplitudes for the frequency given by the index
    /// </summary>
    SoundData ^soundData;
};