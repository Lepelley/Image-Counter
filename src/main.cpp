#include <Windows.h>
#include <objbase.h>
#include <shellscalingapi.h>
#include "ImageCounter.h"

#pragma comment(lib, "shcore.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    // Activer la prise en charge DPI pour éviter les décalages de coordonnées
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    
    // Initialiser COM (pour certaines fonctionnalités Windows)
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    MainWindow app(hInstance);
    
    if (!app.create()) {
        MessageBoxW(nullptr, L"Impossible de créer la fenêtre principale.", 
            L"Erreur", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    int result = app.run();
    
    CoUninitialize();
    return result;
}
