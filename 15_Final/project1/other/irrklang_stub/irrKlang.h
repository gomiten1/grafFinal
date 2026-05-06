#ifndef IRRKLANG_H
#define IRRKLANG_H

namespace irrklang {

enum E_SOUND_OUTPUT_DRIVER
{
    ESDL_AUTO_DETECT = 0
};

class ISoundEngine
{
public:
    void drop() {}
    void play2D(const char*, bool = false) {}
    void play3D(const char*, float = 0.0f, float = 0.0f, float = 0.0f, bool = false) {}
};

inline ISoundEngine* createIrrKlangDevice(E_SOUND_OUTPUT_DRIVER = ESDL_AUTO_DETECT, int = 0, const char* = nullptr, const char* = nullptr)
{
    static ISoundEngine engine;
    return &engine;
}

}

#endif
