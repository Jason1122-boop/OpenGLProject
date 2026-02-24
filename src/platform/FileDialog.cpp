#include "FileDialog.h"
#include <windows.h>
#include <commdlg.h>

std::string OpenFileDialog()
{
    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Model Files\0*.fbx;*.obj\0";
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn))
        return std::string(szFile);

    return "";
}