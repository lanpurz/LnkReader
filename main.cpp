
#include "LnkReader.h"

#define         LINKFILE_PATH_010EDITOR       "C:\\Users\\Administrator\\Desktop\\其他\\lnk\\010 Editor.lnk"
#define         LINKFILE_PATH_PS              "C:\\Users\\Administrator\\Desktop\\其他\\lnk\\Adobe After Effects 2023.lnk"
#define         LINKFILE_PATH_EPIC            "C:\\Users\\Administrator\\Desktop\\其他\\lnk\\Epic Games Launcher.lnk"
#define         LINKFILE_PATH_GOLINK          "C:\\Users\\Administrator\\Desktop\\其他\\lnk\\Go Link.lnk"

int main()
{
    LnkReader lnk;
    lnk.run(R"(C:\Users\Administrator\Desktop\sublime_text.lnk)");

    string path = lnk.getPath();

    return 0;
}
