// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#pragma unmanaged

class MySoundHardware
{

};

#pragma managed
using namespace System;

public ref class SoundHardware
{
    MySoundHardware* inner;
public:
    SoundHardware()
    {
        inner = new MySoundHardware;
    }
    void Begin()
    {
        if (soundData)
            soundData(nullptr);
    }
    void Enqueue(cli::array<System::Byte>^ data)
    {
    }
    delegate void SoundData(cli::array<Byte>^ data);
    SoundData ^soundData;
};