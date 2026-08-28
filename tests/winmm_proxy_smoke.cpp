#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include <cstdio>

int main() {
    const DWORD first = timeGetTime();
    Sleep(2);
    const DWORD second = timeGetTime();
    if (second < first) {
        std::fputs("timeGetTime moved backwards\n", stderr);
        return 1;
    }

    const MMRESULT begin = timeBeginPeriod(1);
    const MMRESULT end = timeEndPeriod(1);
    if (begin != TIMERR_NOERROR || end != TIMERR_NOERROR) {
        std::fputs("WinMM timer forwarding failed\n", stderr);
        return 2;
    }

    WAVEOUTCAPSW capabilities{};
    const MMRESULT wave = waveOutGetDevCapsW(
        WAVE_MAPPER, &capabilities, sizeof(capabilities));
    if (wave != MMSYSERR_NOERROR && wave != MMSYSERR_BADDEVICEID &&
        wave != MMSYSERR_NODRIVER) {
        std::fprintf(stderr, "waveOutGetDevCapsW failed unexpectedly: %u\n", wave);
        return 3;
    }

    std::printf("WinMM proxy smoke test passed; waveOut=%u, waveOutDevices=%u, "
                "waveInDevices=%u, mixers=%u\n",
                wave, waveOutGetNumDevs(), waveInGetNumDevs(), mixerGetNumDevs());
    return 0;
}
