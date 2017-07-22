#ifndef VERSION_H_INCLUDED
#define VERSION_H_INCLUDED

class RTTR_Version
{
public:
    static const char* GetTitle();
    static const char* GetVersion();
    static const char* GetRevision();
    static const char* GetShortRevision();
    static const char* GetYear();
};

#endif // VERSION_H_INCLUDED
