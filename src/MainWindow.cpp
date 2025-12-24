#include "ImageCounter.h"
#include "SettingsDialog.h"
#include <commdlg.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <ShlObj.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// ============================================================================
// RenameDialog Implementation
// ============================================================================

std::wstring* RenameDialog::s_namePtr = nullptr;
HBRUSH RenameDialog::s_darkBrush = nullptr;

bool RenameDialog::show(HWND parent, std::wstring& name) {
    s_namePtr = &name;
    
    const wchar_t* className = L"RenameDialogClass";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DialogProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = ThemeManager::getInstance().isDarkTheme() ? 
        CreateSolidBrush(RGB(32, 32, 32)) : (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);
    
    RECT parentRect;
    GetWindowRect(parent, &parentRect);
    int w = 300, h = 150;
    int x = parentRect.left + (parentRect.right - parentRect.left - w) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - h) / 2;
    
    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        TR("rename_tab"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h,
        parent, nullptr, GetModuleHandle(nullptr), nullptr);
    
    if (!hwnd) return false;
    
    // Appliquer le thème sombre
    ThemeManager::getInstance().applyToWindow(hwnd);
    
    EnableWindow(parent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    MSG msg = {};
    bool running = true;
    
    while (running) {
        // Utiliser MsgWaitForMultipleObjects avec timeout
        DWORD waitResult = MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLINPUT);
        
        if (waitResult == WAIT_TIMEOUT) {
            if (!IsWindow(hwnd)) {
                running = false;
            }
            continue;
        }
        
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (!IsWindow(hwnd)) {
                running = false;
                break;
            }
            
            if (msg.message == WM_QUIT) {
                running = false;
                PostQuitMessage((int)msg.wParam);
                break;
            }
            
            // Gérer Entrée et Échap
            if (msg.message == WM_KEYDOWN && (msg.hwnd == hwnd || IsChild(hwnd, msg.hwnd))) {
                if (msg.wParam == VK_RETURN) {
                    SendMessage(hwnd, WM_COMMAND, IDOK, 0);
                    continue;
                } else if (msg.wParam == VK_ESCAPE) {
                    SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
                    continue;
                }
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    
    if (s_darkBrush) {
        DeleteObject(s_darkBrush);
        s_darkBrush = nullptr;
    }
    
    return (msg.wParam == IDOK);
}

INT_PTR CALLBACK RenameDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND editBox = nullptr;
    static HWND labelName = nullptr;
    static HWND btnOK = nullptr;
    static HWND btnCancel = nullptr;
    
    switch (msg) {
        case WM_CREATE: {
            bool isDark = ThemeManager::getInstance().isDarkTheme();
            
            labelName = CreateWindowExW(0, L"STATIC", TR("new_name"),
                WS_CHILD | WS_VISIBLE, 15, 15, 120, 20, hwnd, nullptr, nullptr, nullptr);
            
            editBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", s_namePtr->c_str(),
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                15, 38, 255, 24, hwnd, (HMENU)1001, nullptr, nullptr);
            
            btnOK = CreateWindowExW(0, L"BUTTON", TR("ok"),
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                95, 75, 80, 28, hwnd, (HMENU)IDOK, nullptr, nullptr);
            
            btnCancel = CreateWindowExW(0, L"BUTTON", TR("cancel"),
                WS_CHILD | WS_VISIBLE,
                185, 75, 80, 28, hwnd, (HMENU)IDCANCEL, nullptr, nullptr);
            
            // Appliquer le thème sombre aux boutons
            if (isDark) {
                SetWindowTheme(btnOK, L"DarkMode_Explorer", nullptr);
                SetWindowTheme(btnCancel, L"DarkMode_Explorer", nullptr);
                SetWindowTheme(editBox, L"DarkMode_CFD", nullptr);
            }
            
            SetFocus(editBox);
            SendMessage(editBox, EM_SETSEL, 0, -1);
            return 0;
        }
        
        case WM_CTLCOLORSTATIC: {
            if (ThemeManager::getInstance().isDarkTheme()) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(32, 32, 32));
                if (!s_darkBrush) s_darkBrush = CreateSolidBrush(RGB(32, 32, 32));
                return (INT_PTR)s_darkBrush;
            }
            break;
        }
        
        case WM_CTLCOLOREDIT: {
            if (ThemeManager::getInstance().isDarkTheme()) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(45, 45, 45));
                static HBRUSH editBrush = CreateSolidBrush(RGB(45, 45, 45));
                return (INT_PTR)editBrush;
            }
            break;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                wchar_t buffer[256];
                GetWindowTextW(editBox, buffer, 256);
                *s_namePtr = buffer;
                DestroyWindow(hwnd);
                PostQuitMessage(IDOK);
            } else if (LOWORD(wParam) == IDCANCEL) {
                DestroyWindow(hwnd);
                PostQuitMessage(IDCANCEL);
            }
            return 0;
            
        case WM_CLOSE:
            DestroyWindow(hwnd);
            PostQuitMessage(IDCANCEL);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// ============================================================================
// MainWindow Implementation
// ============================================================================

MainWindow::MainWindow(HINSTANCE hInstance) : m_hInstance(hInstance) {
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icex);
}

MainWindow::~MainWindow() {
    for (auto& tab : m_tabs) {
        if (tab->detector) {
            tab->detector->stopScanning();
        }
    }
    if (m_counterFont) {
        DeleteObject(m_counterFont);
    }
}

bool MainWindow::create() {
    const wchar_t* className = L"ImageCounterMainWindow";
    
    // Charger l'icône personnalisée
    HICON hIcon = (HICON)LoadImageW(m_hInstance, L"IDI_APPICON", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    HICON hIconSm = (HICON)LoadImageW(m_hInstance, L"IDI_APPICON", IMAGE_ICON, 16, 16, LR_DEFAULTSIZE);
    
    // Fallback si l'icône n'est pas trouvée
    if (!hIcon) hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    if (!hIconSm) hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    wc.hIcon = hIcon;
    wc.hIconSm = hIconSm;
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    m_hwnd = CreateWindowExW(
        0, className,
        TR("app_title"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        680, 600,
        nullptr, nullptr, m_hInstance, this);
    
    if (!m_hwnd) {
        return false;
    }
    
    // Appliquer le thème
    ThemeManager::getInstance().applyToWindow(m_hwnd);
    
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    return true;
}

int MainWindow::run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        switch (msg) {
            case WM_CREATE:
                pThis->onCreate(hwnd);
                return 0;
            case WM_COMMAND:
                pThis->onCommand(hwnd, wParam);
                return 0;
            case WM_NOTIFY:
                pThis->onNotify(hwnd, lParam);
                return 0;
            case WM_PAINT:
                pThis->onPaint(hwnd);
                return 0;
            case WM_TIMER:
                pThis->onTimer(hwnd);
                return 0;
            
            // Gestion du thème sombre pour les contrôles statiques
            case WM_CTLCOLORSTATIC: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkMode(hdc, TRANSPARENT);
                    if (!pThis->m_backgroundBrush) {
                        pThis->m_backgroundBrush = CreateSolidBrush(RGB(32, 32, 32));
                    }
                    return (LRESULT)pThis->m_backgroundBrush;
                }
                break;
            }
            
            // Gestion du thème sombre pour les boutons (radio, checkbox)
            case WM_CTLCOLORBTN: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkMode(hdc, TRANSPARENT);
                    if (!pThis->m_backgroundBrush) {
                        pThis->m_backgroundBrush = CreateSolidBrush(RGB(32, 32, 32));
                    }
                    return (LRESULT)pThis->m_backgroundBrush;
                }
                break;
            }
            
            // Couleur pour les edits (partie texte des combos)
            case WM_CTLCOLOREDIT: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(45, 45, 45));
                    if (!pThis->m_backgroundBrush) {
                        pThis->m_backgroundBrush = CreateSolidBrush(RGB(32, 32, 32));
                    }
                    static HBRUSH editBrush = CreateSolidBrush(RGB(45, 45, 45));
                    return (LRESULT)editBrush;
                }
                break;
            }
            
            // Couleur pour les listes déroulantes
            case WM_CTLCOLORLISTBOX: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(45, 45, 45));
                    static HBRUSH listBrush = CreateSolidBrush(RGB(45, 45, 45));
                    return (LRESULT)listBrush;
                }
                break;
            }
            
            // Fond de la fenêtre
            case WM_ERASEBKGND: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    if (!pThis->m_backgroundBrush) {
                        pThis->m_backgroundBrush = CreateSolidBrush(RGB(32, 32, 32));
                    }
                    FillRect(hdc, &rect, pThis->m_backgroundBrush);
                    return 1;
                }
                break;
            }
            
            // Owner draw pour le Tab Control
            case WM_DRAWITEM: {
                auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (dis->CtlID == IDC_TAB_CONTROL) {
                    HDC hdc = dis->hDC;
                    RECT rc = dis->rcItem;
                    bool isSelected = (dis->itemState & ODS_SELECTED);
                    bool isDark = ThemeManager::getInstance().isDarkTheme();
                    
                    // Couleurs selon le thème
                    COLORREF bgColor, textColor;
                    if (isDark) {
                        bgColor = isSelected ? RGB(50, 50, 50) : RGB(40, 40, 40);
                        textColor = RGB(255, 255, 255);
                    } else {
                        bgColor = isSelected ? GetSysColor(COLOR_WINDOW) : GetSysColor(COLOR_BTNFACE);
                        textColor = GetSysColor(COLOR_WINDOWTEXT);
                    }
                    
                    // Dessiner le fond
                    HBRUSH brush = CreateSolidBrush(bgColor);
                    FillRect(hdc, &rc, brush);
                    DeleteObject(brush);
                    
                    // Dessiner le texte
                    SetTextColor(hdc, textColor);
                    SetBkMode(hdc, TRANSPARENT);
                    
                    // Obtenir le texte de l'onglet
                    TCITEMW item = {};
                    wchar_t text[256] = {};
                    item.mask = TCIF_TEXT;
                    item.pszText = text;
                    item.cchTextMax = 256;
                    TabCtrl_GetItem(pThis->m_tabControl, dis->itemID, &item);
                    
                    // Ajuster le rect pour centrer le texte
                    rc.top += 2;
                    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                    
                    return TRUE;
                }
                break;
            }
            
            case WM_HOTKEY:
                pThis->onHotkey(wParam);
                return 0;
            
            case WM_CLOSE:
                // Sauvegarder les paramètres avant de fermer
                pThis->saveAllSettings();
                DestroyWindow(hwnd);
                return 0;
            case WM_DESTROY:
                KillTimer(hwnd, IDT_UPDATE);
                if (pThis->m_backgroundBrush) {
                    DeleteObject(pThis->m_backgroundBrush);
                    pThis->m_backgroundBrush = nullptr;
                }
                PostQuitMessage(0);
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MainWindow::onCreate(HWND hwnd) {
    int y = 10;
    const int margin = 15;
    const int windowWidth = 620;
    const int btnHeight = 32;
    
    // Créer la police pour les boutons (Segoe UI Emoji pour bien afficher les emojis)
    m_buttonFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");
    
    // === Barre d'onglets ===
    // Boutons de gestion des onglets à droite (+ et -)
    int btnX = windowWidth - margin - 60;
    m_btnAddTab = CreateWindowExW(0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        btnX, y, 28, 26,
        hwnd, (HMENU)IDC_BTN_ADD_TAB, m_hInstance, nullptr);
    
    m_btnRemoveTab = CreateWindowExW(0, L"BUTTON", L"−",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        btnX + 30, y, 28, 26,
        hwnd, (HMENU)IDC_BTN_REMOVE_TAB, m_hInstance, nullptr);
    
    // Tab Control (plus large maintenant)
    m_tabControl = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_OWNERDRAWFIXED,
        margin, y, btnX - margin - 10, 28,
        hwnd, (HMENU)IDC_TAB_CONTROL, m_hInstance, nullptr);
    
    // Subclasser le Tab Control pour dessiner le fond sombre
    SetWindowSubclass(m_tabControl, [](HWND hWnd, UINT uMsg, WPARAM wParam, 
        LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT {
        
        if (uMsg == WM_ERASEBKGND && ThemeManager::getInstance().isDarkTheme()) {
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hWnd, &rect);
            HBRUSH brush = CreateSolidBrush(RGB(32, 32, 32));
            FillRect(hdc, &rect, brush);
            DeleteObject(brush);
            return 1;
        }
        
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }, 1, 0);
    
    y += 38;
    
    // === Zone de contenu ===
    
    // Fenêtre cible
    m_labelTargetWindow = CreateWindowExW(0, L"STATIC", TR("target_window"),
        WS_CHILD | WS_VISIBLE, margin, y, 120, 20, hwnd, nullptr, m_hInstance, nullptr);
    y += 22;
    
    m_comboWindows = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        margin, y, windowWidth - 105, 200,
        hwnd, (HMENU)IDC_COMBO_WINDOWS, m_hInstance, nullptr);
    
    m_btnRefreshWindows = CreateWindowExW(0, L"BUTTON", L"🔄",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        windowWidth - 80, y - 2, 36, 28,
        hwnd, (HMENU)IDC_BTN_REFRESH_WINDOWS, m_hInstance, nullptr);
    
    y += 35;
    
    // Boutons d'action - Ligne 1 (3 boutons répartis sur toute la largeur)
    int btn3Width = (windowWidth - 20) / 3;
    
    m_btnLoadImage = CreateWindowExW(0, L"BUTTON", TR("load_image"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin, y, btn3Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_LOAD_IMAGE, m_hInstance, nullptr);
    
    m_btnQuickCapture = CreateWindowExW(0, L"BUTTON", TR("quick_capture"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + btn3Width + 10, y, btn3Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_QUICK_CAPTURE, m_hInstance, nullptr);
    
    m_btnSelectRegion = CreateWindowExW(0, L"BUTTON", TR("select_zone"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + (btn3Width + 10) * 2, y, btn3Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_SELECT_REGION, m_hInstance, nullptr);
    
    y += btnHeight + 8;
    
    // Boutons d'action - Ligne 2 (Paramètres et Historique)
    int btn2Width = (windowWidth - 10) / 2;
    
    m_btnSettings = CreateWindowExW(0, L"BUTTON", TR("settings"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin, y, btn2Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_SETTINGS, m_hInstance, nullptr);
    
    m_btnHistory = CreateWindowExW(0, L"BUTTON", TR("history"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + btn2Width + 10, y, btn2Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_HISTORY, m_hInstance, nullptr);
    
    y += btnHeight + 15;
    
    // Séparateur
    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        margin, y, windowWidth, 2, hwnd, nullptr, m_hInstance, nullptr);
    y += 12;
    
    // === Compteur ===
    m_labelCounter = CreateWindowExW(0, L"STATIC", TR("counter_label"),
        WS_CHILD | WS_VISIBLE, margin, y + 15, 80, 20, hwnd, nullptr, m_hInstance, nullptr);
    
    // Bouton -1
    m_btnCounterMinus = CreateWindowExW(0, L"BUTTON", L"−1",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + 85, y + 15, 35, 30,
        hwnd, (HMENU)IDC_BTN_COUNTER_MINUS, m_hInstance, nullptr);
    
    m_staticCounter = CreateWindowExW(0, L"STATIC", L"0",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOTIFY,
        margin + 125, y, 110, 60,
        hwnd, (HMENU)IDC_STATIC_COUNTER, m_hInstance, nullptr);
    
    // Bouton +1
    m_btnCounterPlus = CreateWindowExW(0, L"BUTTON", L"+1",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + 240, y + 15, 35, 30,
        hwnd, (HMENU)IDC_BTN_COUNTER_PLUS, m_hInstance, nullptr);
    
    // Affichage du pas
    m_staticStep = CreateWindowExW(0, L"STATIC", L"(+1)",
        WS_CHILD | WS_VISIBLE,
        margin + 280, y + 20, 50, 20,
        hwnd, (HMENU)IDC_STATIC_STEP, m_hInstance, nullptr);
    
    m_counterFont = CreateFontW(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessage(m_staticCounter, WM_SETFONT, (WPARAM)m_counterFont, TRUE);
    
    y += 65;
    
    // Boutons de contrôle (3 boutons répartis)
    m_btnStart = CreateWindowExW(0, L"BUTTON", TR("start"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin, y, btn3Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_START, m_hInstance, nullptr);
    
    m_btnStop = CreateWindowExW(0, L"BUTTON", TR("stop"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        margin + btn3Width + 10, y, btn3Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_STOP, m_hInstance, nullptr);
    
    m_btnReset = CreateWindowExW(0, L"BUTTON", TR("reset"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        margin + (btn3Width + 10) * 2, y, btn3Width, btnHeight,
        hwnd, (HMENU)IDC_BTN_RESET, m_hInstance, nullptr);
    
    y += btnHeight + 15;
    
    // Statut
    m_staticStatus = CreateWindowExW(0, L"STATIC", TR("status_waiting"),
        WS_CHILD | WS_VISIBLE,
        margin, y, windowWidth, 20,
        hwnd, (HMENU)IDC_STATIC_STATUS, m_hInstance, nullptr);
    y += 28;
    
    // Prévisualisations côte à côte
    int previewWidth = (windowWidth - 20) / 2;
    int previewHeight = 130;
    
    // Image de référence (gauche)
    m_labelRefImage = CreateWindowExW(0, L"STATIC", TR("reference_image"),
        WS_CHILD | WS_VISIBLE, margin, y, 180, 20, hwnd, nullptr, m_hInstance, nullptr);
    
    // Capture en cours (droite)
    m_labelCapture = CreateWindowExW(0, L"STATIC", TR("current_capture"),
        WS_CHILD | WS_VISIBLE, margin + previewWidth + 20, y, 150, 20, hwnd, nullptr, m_hInstance, nullptr);
    
    y += 24;
    
    m_staticPreview = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
        margin, y, previewWidth, previewHeight,
        hwnd, (HMENU)IDC_STATIC_PREVIEW, m_hInstance, nullptr);
    
    m_staticCapture = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
        margin + previewWidth + 20, y, previewWidth, previewHeight,
        hwnd, (HMENU)IDC_STATIC_CAPTURE, m_hInstance, nullptr);
    
    // Appliquer la police aux boutons pour mieux afficher les emojis
    SendMessage(m_btnLoadImage, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnQuickCapture, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnSelectRegion, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnSettings, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnHistory, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnRefreshWindows, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnStart, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnStop, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnReset, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnRenameTab, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnCounterMinus, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    SendMessage(m_btnCounterPlus, WM_SETFONT, (WPARAM)m_buttonFont, TRUE);
    
    // Timer pour mise à jour
    SetTimer(hwnd, IDT_UPDATE, 100, nullptr);
    
    // Remplir la liste des fenêtres
    updateWindowList();
    
    // Charger les paramètres sauvegardés
    loadAllSettings();
    
    // Si aucun onglet n'a été chargé, créer le premier
    if (m_tabs.empty()) {
        addNewTab();
    }
    
    // Appliquer le thème
    applyTheme();
}

void MainWindow::onCommand(HWND hwnd, WPARAM wParam) {
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);
    
    switch (wmId) {
        case IDC_BTN_ADD_TAB:
            addNewTab();
            break;
        case IDC_BTN_REMOVE_TAB:
            removeCurrentTab();
            break;
        case IDC_COMBO_WINDOWS:
            if (wmEvent == CBN_SELCHANGE) {
                int index = (int)SendMessage(m_comboWindows, CB_GETCURSEL, 0, 0);
                if (auto* tab = currentTab()) {
                    tab->selectedWindowIndex = index;
                    if (index >= 0 && index < (int)m_windowList.size()) {
                        tab->detector->setTargetWindow(m_windowList[index].first);
                        tab->targetWindow = m_windowList[index].first;
                        tab->targetWindowTitle = m_windowList[index].second;
                    }
                }
                SetWindowTextW(m_staticStatus, L"Statut: Fenêtre sélectionnée");
            }
            break;
        case IDC_BTN_REFRESH_WINDOWS:
            updateWindowList();
            break;
        case IDC_BTN_LOAD_IMAGE:
            loadReferenceImage();
            break;
        case IDC_BTN_QUICK_CAPTURE:
            quickCapture();
            break;
        case IDC_BTN_SELECT_REGION:
            selectCaptureRegion();
            break;
        case IDC_BTN_SETTINGS:
            showSettings();
            break;
        case IDC_BTN_HISTORY:
            showHistory();
            break;
        case IDC_BTN_START:
            startScanning();
            break;
        case IDC_BTN_STOP:
            stopScanning();
            break;
        case IDC_BTN_RESET:
            resetCounter();
            break;
        case IDC_BTN_COUNTER_MINUS:
            if (auto* tab = currentTab()) {
                int counter = tab->detector->getCounter();
                if (counter > 0) {
                    counter--;
                    tab->detector->setCounter(counter);
                    SetWindowTextW(m_staticCounter, std::to_wstring(counter).c_str());
                    updateTabControl();
                    saveCounterToFile(tab);
                }
            }
            break;
        case IDC_BTN_COUNTER_PLUS:
            if (auto* tab = currentTab()) {
                int counter = tab->detector->getCounter();
                counter++;
                tab->detector->setCounter(counter);
                SetWindowTextW(m_staticCounter, std::to_wstring(counter).c_str());
                updateTabControl();
                saveCounterToFile(tab);
            }
            break;
        case IDC_STATIC_COUNTER:
            if (wmEvent == STN_CLICKED) {
                editCounter();
            }
            break;
    }
}

void MainWindow::onNotify(HWND hwnd, LPARAM lParam) {
    auto* nmhdr = reinterpret_cast<NMHDR*>(lParam);
    
    if (nmhdr->idFrom == IDC_TAB_CONTROL && nmhdr->code == TCN_SELCHANGE) {
        int sel = TabCtrl_GetCurSel(m_tabControl);
        switchToTab(sel);
    }
}

void MainWindow::onPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    EndPaint(hwnd, &ps);
}

void MainWindow::onTimer(HWND hwnd) {
    // Mettre à jour tous les compteurs actifs
    for (size_t i = 0; i < m_tabs.size(); i++) {
        auto& tab = m_tabs[i];
        if (tab->isRunning) {
            // Mettre à jour le nom de l'onglet avec le compteur
            int count = tab->detector->getCounter();
            std::wstring tabName = tab->name + L" (" + std::to_wstring(count) + L")";
            
            TCITEMW tie = {};
            tie.mask = TCIF_TEXT;
            tie.pszText = const_cast<LPWSTR>(tabName.c_str());
            TabCtrl_SetItem(m_tabControl, (int)i, &tie);
        }
    }
    
    // Mettre à jour l'affichage du compteur courant et le score
    if (auto* tab = currentTab()) {
        int counter = tab->detector->getCounter();
        SetWindowTextW(m_staticCounter, std::to_wstring(counter).c_str());
        
        // Afficher le score de détection si le scan est actif
        if (tab->isRunning) {
            double confidence = tab->detector->getLastConfidence();
            double threshold = tab->config.threshold;
            int confidencePercent = (int)(confidence * 100);
            int thresholdPercent = (int)(threshold * 100);
            
            std::wstringstream ss;
            ss << L"Score: " << confidencePercent 
               << L"% (seuil: " << thresholdPercent << L"%)";
            
            // Colorer différemment si au-dessus du seuil
            if (confidence >= threshold) {
                ss << L" ✓";
            }
            
            // Afficher le temps depuis la dernière détection
            if (tab->lastDetectionTime > 0) {
                ULONGLONG elapsed = GetTickCount64() - tab->lastDetectionTime;
                int seconds = (int)(elapsed / 1000);
                
                if (seconds < 60) {
                    ss << L" | Dernière détection: il y a " << seconds << L"s";
                } else if (seconds < 3600) {
                    int minutes = seconds / 60;
                    ss << L" | Dernière détection: il y a " << minutes << L"min";
                } else {
                    int hours = seconds / 3600;
                    int mins = (seconds % 3600) / 60;
                    ss << L" | Dernière détection: il y a " << hours << L"h" << mins << L"m";
                }
            } else {
                ss << L" | Aucune détection";
            }
            
            SetWindowTextW(m_staticStatus, ss.str().c_str());
            
            // Mettre à jour la prévisualisation de la capture
            updateCapturePreview(tab);
        }
    }
}

void MainWindow::onDetection(int tabIndex, const DetectionResult& result, int counter) {
    if (tabIndex < 0 || tabIndex >= (int)m_tabs.size()) return;
    
    auto& tab = m_tabs[tabIndex];
    
    // Enregistrer le temps de la détection
    tab->lastDetectionTime = GetTickCount64();
    
    // Ajouter à l'historique
    addDetectionToHistory(tab.get(), counter, result.confidence);
    
    // Jouer le son si activé
    if (tab->soundEnabled) {
        switch (tab->soundIndex) {
            case 0: MessageBeep(MB_OK); break;
            case 1: MessageBeep(MB_ICONASTERISK); break;
            case 2: PlaySoundW(L"SystemNotification", nullptr, SND_ALIAS | SND_ASYNC); break;
        }
    }
    
    // Sauvegarder le compteur dans le fichier
    saveCounterToFile(tab.get());
    
    PostMessage(m_hwnd, WM_TIMER, 0, 0);
}

// ============================================================================
// Gestion des onglets
// ============================================================================

void MainWindow::addNewTab() {
    auto tab = std::make_unique<CounterTab>();
    
    int tabNum = (int)m_tabs.size() + 1;
    tab->name = L"Compteur " + std::to_wstring(tabNum);
    
    // Configurer le callback
    int tabIndex = (int)m_tabs.size();
    tab->detector->setDetectionCallback([this, tabIndex](const DetectionResult& result, int counter) {
        onDetection(tabIndex, result, counter);
    });
    
    // Charger le compteur depuis le fichier s'il existe
    loadCounterFromFile(tab.get());
    
    m_tabs.push_back(std::move(tab));
    
    // Ajouter l'onglet au contrôle
    TCITEMW tie = {};
    tie.mask = TCIF_TEXT;
    tie.pszText = const_cast<LPWSTR>(m_tabs.back()->name.c_str());
    TabCtrl_InsertItem(m_tabControl, (int)m_tabs.size() - 1, &tie);
    
    // Sélectionner le nouvel onglet
    TabCtrl_SetCurSel(m_tabControl, (int)m_tabs.size() - 1);
    switchToTab((int)m_tabs.size() - 1);
}

void MainWindow::removeCurrentTab() {
    if (m_tabs.size() <= 1) {
        MessageBoxW(m_hwnd, TR("error_need_one_tab"),
            TR("warning"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Confirmation traduite
    const wchar_t* confirmMsg = Localization::getInstance().getLanguage() == Language::French ?
        L"Voulez-vous vraiment supprimer cet onglet ?" :
        L"Do you really want to delete this tab?";
    const wchar_t* confirmTitle = Localization::getInstance().getLanguage() == Language::French ?
        L"Confirmation" : L"Confirmation";
    
    int result = MessageBoxW(m_hwnd, confirmMsg, confirmTitle, MB_YESNO | MB_ICONQUESTION);
    
    if (result != IDYES) return;
    
    // Arrêter le scan si actif
    if (auto* tab = currentTab()) {
        tab->detector->stopScanning();
    }
    
    // Supprimer l'onglet
    TabCtrl_DeleteItem(m_tabControl, m_currentTabIndex);
    m_tabs.erase(m_tabs.begin() + m_currentTabIndex);
    
    // Ajuster l'index
    if (m_currentTabIndex >= (int)m_tabs.size()) {
        m_currentTabIndex = (int)m_tabs.size() - 1;
    }
    
    TabCtrl_SetCurSel(m_tabControl, m_currentTabIndex);
    switchToTab(m_currentTabIndex);
}

void MainWindow::switchToTab(int index) {
    if (index < 0 || index >= (int)m_tabs.size()) return;
    
    m_currentTabIndex = index;
    updateUIForCurrentTab();
}

void MainWindow::updateTabControl() {
    for (size_t i = 0; i < m_tabs.size(); i++) {
        auto& tab = m_tabs[i];
        std::wstring displayName = tab->name;
        
        if (tab->isRunning) {
            displayName += L" (" + std::to_wstring(tab->detector->getCounter()) + L")";
        }
        
        TCITEMW tie = {};
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<LPWSTR>(displayName.c_str());
        TabCtrl_SetItem(m_tabControl, (int)i, &tie);
    }
}

CounterTab* MainWindow::currentTab() {
    if (m_currentTabIndex >= 0 && m_currentTabIndex < (int)m_tabs.size()) {
        return m_tabs[m_currentTabIndex].get();
    }
    return nullptr;
}

void MainWindow::updateUIForCurrentTab() {
    auto* tab = currentTab();
    if (!tab) return;
    
    // Mettre à jour la sélection de fenêtre
    if (tab->selectedWindowIndex >= 0 && tab->selectedWindowIndex < (int)m_windowList.size()) {
        SendMessage(m_comboWindows, CB_SETCURSEL, tab->selectedWindowIndex, 0);
    } else {
        SendMessage(m_comboWindows, CB_SETCURSEL, -1, 0);
    }
    
    // Mettre à jour le compteur
    SetWindowTextW(m_staticCounter, std::to_wstring(tab->detector->getCounter()).c_str());
    
    // Mettre à jour l'affichage du pas
    std::wstring stepText = L"(+" + std::to_wstring(tab->config.counterStep) + L")";
    SetWindowTextW(m_staticStep, stepText.c_str());
    
    // Mettre à jour la prévisualisation
    if (tab->previewBitmap) {
        SendMessage(m_staticPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)tab->previewBitmap);
    } else {
        SendMessage(m_staticPreview, STM_SETIMAGE, IMAGE_BITMAP, 0);
    }
    
    // Mettre à jour la prévisualisation de la capture
    if (tab->captureBitmap) {
        SendMessage(m_staticCapture, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)tab->captureBitmap);
    } else {
        SendMessage(m_staticCapture, STM_SETIMAGE, IMAGE_BITMAP, 0);
    }
    
    // Mettre à jour les boutons
    EnableWindow(m_btnStart, !tab->isRunning);
    EnableWindow(m_btnStop, tab->isRunning);
    
    // Mettre à jour le statut
    if (tab->isRunning) {
        SetWindowTextW(m_staticStatus, TR("status_scanning"));
    } else if (tab->detector->hasReferenceImage()) {
        SetWindowTextW(m_staticStatus, TR("status_ready"));
    } else {
        SetWindowTextW(m_staticStatus, TR("status_waiting_reference"));
    }
}

void MainWindow::updateWindowList() {
    SendMessage(m_comboWindows, CB_RESETCONTENT, 0, 0);
    m_windowList = ImageDetector::enumerateWindows();
    
    for (const auto& [hwnd, title] : m_windowList) {
        std::wstring displayText = title;
        if (displayText.length() > 60) {
            displayText = displayText.substr(0, 57) + L"...";
        }
        SendMessageW(m_comboWindows, CB_ADDSTRING, 0, (LPARAM)displayText.c_str());
    }
    
    // Restaurer la sélection pour l'onglet courant
    if (auto* tab = currentTab()) {
        if (tab->selectedWindowIndex >= 0) {
            SendMessage(m_comboWindows, CB_SETCURSEL, tab->selectedWindowIndex, 0);
        }
    }
}

void MainWindow::updatePreviewForTab(CounterTab* tab, const cv::Mat& image) {
    if (!tab || image.empty()) return;
    
    // Supprimer l'ancien bitmap
    if (tab->previewBitmap) {
        DeleteObject(tab->previewBitmap);
        tab->previewBitmap = nullptr;
    }
    
    // Redimensionner pour la prévisualisation (300x130)
    cv::Mat preview;
    double scale = (std::min)(300.0 / image.cols, 130.0 / image.rows);
    cv::resize(image, preview, cv::Size(), scale, scale);
    
    // Convertir en HBITMAP
    cv::Mat bgra;
    cv::cvtColor(preview, bgra, cv::COLOR_BGR2BGRA);
    
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = preview.cols;
    bmi.bmiHeader.biHeight = -preview.rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    HDC hdc = GetDC(m_staticPreview);
    tab->previewBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT,
        bgra.data, &bmi, DIB_RGB_COLORS);
    ReleaseDC(m_staticPreview, hdc);
    
    // Afficher si c'est l'onglet courant
    if (tab == currentTab()) {
        SendMessage(m_staticPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)tab->previewBitmap);
    }
}

void MainWindow::updateCapturePreview(CounterTab* tab) {
    if (!tab || !tab->isRunning) return;
    
    // Capturer l'image actuelle
    cv::Mat capture = tab->detector->captureRegion();
    if (capture.empty()) return;
    
    // Supprimer l'ancien bitmap
    if (tab->captureBitmap) {
        DeleteObject(tab->captureBitmap);
        tab->captureBitmap = nullptr;
    }
    
    // Redimensionner pour la prévisualisation (300x130)
    cv::Mat preview;
    double scale = (std::min)(300.0 / capture.cols, 130.0 / capture.rows);
    cv::resize(capture, preview, cv::Size(), scale, scale);
    
    // Convertir en HBITMAP
    cv::Mat bgra;
    cv::cvtColor(preview, bgra, cv::COLOR_BGR2BGRA);
    
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = preview.cols;
    bmi.bmiHeader.biHeight = -preview.rows;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    HDC hdc = GetDC(m_staticCapture);
    tab->captureBitmap = CreateDIBitmap(hdc, &bmi.bmiHeader, CBM_INIT,
        bgra.data, &bmi, DIB_RGB_COLORS);
    ReleaseDC(m_staticCapture, hdc);
    
    // Afficher si c'est l'onglet courant
    if (tab == currentTab()) {
        SendMessage(m_staticCapture, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)tab->captureBitmap);
    }
}

// ============================================================================
// Actions
// ============================================================================

void MainWindow::loadReferenceImage() {
    auto* tab = currentTab();
    if (!tab) return;
    
    wchar_t filename[MAX_PATH] = {};
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Images\0*.png;*.jpg;*.jpeg;*.bmp\0Tous les fichiers\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrTitle = L"Sélectionner l'image de référence";
    
    if (GetOpenFileNameW(&ofn)) {
        if (tab->detector->loadReferenceImage(filename)) {
            tab->referenceImagePath = filename;  // Sauvegarder le chemin
            SetWindowTextW(m_staticStatus, TR("status_image_loaded"));
            updatePreviewForTab(tab, tab->detector->getReferenceImage());
        } else {
            MessageBoxW(m_hwnd, TR("error_load_image"), TR("error"), MB_OK | MB_ICONERROR);
        }
    }
}

void MainWindow::quickCapture() {
    auto* tab = currentTab();
    if (!tab) return;
    
    HWND target = tab->detector->getTargetWindow();
    if (!target) {
        MessageBoxW(m_hwnd, TR("error_no_window"),
            TR("error"), MB_OK | MB_ICONWARNING);
        return;
    }
    
    ShowWindow(m_hwnd, SW_HIDE);
    Sleep(200);
    
    RegionSelector selector(target);
    if (selector.select()) {
        CaptureRegion region = selector.getRegion();
        
        cv::Mat fullCapture = tab->detector->captureWindow();
        if (!fullCapture.empty()) {
            int x = (std::max)(0, region.x);
            int y = (std::max)(0, region.y);
            int w = (std::min)(region.width, fullCapture.cols - x);
            int h = (std::min)(region.height, fullCapture.rows - y);
            
            if (w > 0 && h > 0) {
                cv::Mat capturedRegion = fullCapture(cv::Rect(x, y, w, h)).clone();
                tab->detector->setReferenceFromMat(capturedRegion);
                updatePreviewForTab(tab, capturedRegion);
                
                std::wstringstream ss;
                ss << L"Statut: Image capturée (" << w << L"x" << h << L")";
                SetWindowTextW(m_staticStatus, ss.str().c_str());
            }
        }
    }
    
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
}

void MainWindow::selectCaptureRegion() {
    auto* tab = currentTab();
    if (!tab) return;
    
    HWND target = tab->detector->getTargetWindow();
    if (!target) {
        MessageBoxW(m_hwnd, TR("error_no_window"),
            TR("error"), MB_OK | MB_ICONWARNING);
        return;
    }
    
    ShowWindow(m_hwnd, SW_HIDE);
    Sleep(200);
    
    RegionSelector selector(target);
    if (selector.select()) {
        tab->region = selector.getRegion();
        tab->detector->setCaptureRegion(tab->region);
        
        // Status traduit
        if (Localization::getInstance().getLanguage() == Language::French) {
            std::wstringstream ss;
            ss << L"Statut: Zone définie (" << tab->region.width << L"x" << tab->region.height << L")";
            SetWindowTextW(m_staticStatus, ss.str().c_str());
        } else {
            std::wstringstream ss;
            ss << L"Status: Region defined (" << tab->region.width << L"x" << tab->region.height << L")";
            SetWindowTextW(m_staticStatus, ss.str().c_str());
        }
    }
    
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
}

void MainWindow::showSettings(int initialTab) {
    auto* tab = currentTab();
    if (!tab) return;
    
    // Désenregistrer le hotkey actuel pendant l'édition
    unregisterHotkeys();
    
    ExtendedConfig config;
    config.detection = tab->config;
    config.region = tab->region;
    config.counterName = tab->name;
    config.soundEnabled = tab->soundEnabled;
    config.soundIndex = tab->soundIndex;
    config.useGrayscale = tab->config.useGrayscale;
    config.matchMethod = tab->config.matchMethod;
    config.savePath = tab->saveFilePath;
    config.hotkeyVirtualKey = tab->hotkeyVirtualKey;
    config.hotkeyModifiers = tab->hotkeyModifiers;
    
    SettingsDialog dialog(m_hwnd, config, tab->detector->getTargetWindow());
    dialog.setInitialTab(initialTab);
    
    if (dialog.show()) {
        config = dialog.getConfig();
        tab->config = config.detection;
        tab->config.useGrayscale = config.useGrayscale;
        tab->config.matchMethod = config.matchMethod;
        tab->region = config.region;
        tab->soundEnabled = config.soundEnabled;
        tab->soundIndex = config.soundIndex;
        tab->saveFilePath = config.savePath;
        tab->hotkeyVirtualKey = config.hotkeyVirtualKey;
        tab->hotkeyModifiers = config.hotkeyModifiers;
        
        // Mettre à jour le nom si changé
        if (!config.counterName.empty() && config.counterName != tab->name) {
            tab->name = config.counterName;
            updateTabControl();
        }
        
        tab->detector->setConfig(tab->config);
        tab->detector->setCaptureRegion(tab->region);
        
        // Mettre à jour l'affichage du pas
        std::wstring stepText = L"(+" + std::to_wstring(tab->config.counterStep) + L")";
        SetWindowTextW(m_staticStep, stepText.c_str());
        
        // Rafraîchir l'UI pour la langue et le thème
        refreshUILanguage();
        applyTheme();
        
        SetWindowTextW(m_staticStatus, TR("status_settings_updated"));
    }
    
    // Réenregistrer le hotkey
    registerHotkeys();
}

void MainWindow::startScanning() {
    auto* tab = currentTab();
    if (!tab) return;
    
    if (!tab->detector->hasReferenceImage()) {
        MessageBoxW(m_hwnd, TR("error_no_reference"),
            TR("error"), MB_OK | MB_ICONWARNING);
        return;
    }
    if (!tab->detector->getTargetWindow()) {
        MessageBoxW(m_hwnd, TR("error_no_window"),
            TR("error"), MB_OK | MB_ICONWARNING);
        return;
    }
    
    tab->detector->startScanning();
    tab->isRunning = true;
    
    EnableWindow(m_btnStart, FALSE);
    EnableWindow(m_btnStop, TRUE);
    SetWindowTextW(m_staticStatus, TR("status_scanning"));
    
    updateTabControl();
}

void MainWindow::stopScanning() {
    auto* tab = currentTab();
    if (!tab) return;
    
    tab->detector->stopScanning();
    tab->isRunning = false;
    
    EnableWindow(m_btnStart, TRUE);
    EnableWindow(m_btnStop, FALSE);
    SetWindowTextW(m_staticStatus, TR("status_stopped"));
    
    updateTabControl();
}

void MainWindow::resetCounter() {
    auto* tab = currentTab();
    if (!tab) return;
    
    tab->detector->resetCounter();
    SetWindowTextW(m_staticCounter, L"0");
    updateTabControl();
    
    // Sauvegarder dans le fichier
    saveCounterToFile(tab);
}

void MainWindow::editCounter() {
    auto* tab = currentTab();
    if (!tab) return;
    
    // Créer une boîte de dialogue pour modifier le compteur
    const wchar_t* className = L"EditCounterDialogClass";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);
    
    RECT parentRect;
    GetWindowRect(m_hwnd, &parentRect);
    int w = 280, h = 150;
    int x = parentRect.left + (parentRect.right - parentRect.left - w) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - h) / 2;
    
    HWND hwndDialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        TR("edit_counter_title"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    if (!hwndDialog) return;
    
    // Créer les contrôles
    CreateWindowExW(0, L"STATIC", TR("new_value"),
        WS_CHILD | WS_VISIBLE, 15, 20, 120, 20, hwndDialog, nullptr, m_hInstance, nullptr);
    
    HWND editBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 
        std::to_wstring(tab->detector->getCounter()).c_str(),
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_RIGHT,
        140, 18, 100, 24, hwndDialog, (HMENU)1001, m_hInstance, nullptr);
    
    HWND btnOK = CreateWindowExW(0, L"BUTTON", L"OK",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        55, 70, 80, 28, hwndDialog, (HMENU)IDOK, m_hInstance, nullptr);
    
    HWND btnCancel = CreateWindowExW(0, L"BUTTON", TR("cancel"),
        WS_CHILD | WS_VISIBLE,
        145, 70, 80, 28, hwndDialog, (HMENU)IDCANCEL, m_hInstance, nullptr);
    
    // Sélectionner tout le texte
    SetFocus(editBox);
    SendMessage(editBox, EM_SETSEL, 0, -1);
    
    EnableWindow(m_hwnd, FALSE);
    ShowWindow(hwndDialog, SW_SHOW);
    UpdateWindow(hwndDialog);
    
    // Boucle de messages avec timeout pour éviter les blocages
    MSG msg;
    bool done = false;
    int result = IDCANCEL;
    
    while (!done) {
        // Utiliser PeekMessage avec un timeout pour éviter les blocages
        DWORD waitResult = MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLINPUT);
        
        if (waitResult == WAIT_TIMEOUT) {
            // Vérifier si le dialog existe toujours
            if (!IsWindow(hwndDialog)) {
                done = true;
                continue;
            }
            continue;
        }
        
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            // Vérifier si le dialog existe toujours
            if (!IsWindow(hwndDialog)) {
                done = true;
                break;
            }
            
            // Gérer les touches Entrée et Échap
            if (msg.message == WM_KEYDOWN && (msg.hwnd == hwndDialog || IsChild(hwndDialog, msg.hwnd))) {
                if (msg.wParam == VK_RETURN) {
                    wchar_t buffer[64];
                    GetWindowTextW(editBox, buffer, 64);
                    int newValue = _wtoi(buffer);
                    if (newValue >= 0) {
                        tab->detector->setCounter(newValue);
                        SetWindowTextW(m_staticCounter, std::to_wstring(newValue).c_str());
                        updateTabControl();
                        saveCounterToFile(tab);
                    }
                    result = IDOK;
                    done = true;
                    break;
                } else if (msg.wParam == VK_ESCAPE) {
                    result = IDCANCEL;
                    done = true;
                    break;
                }
            }
            
            // Gérer les clics sur les boutons
            if (msg.message == WM_LBUTTONUP && (msg.hwnd == btnOK || msg.hwnd == btnCancel)) {
                if (msg.hwnd == btnOK) {
                    wchar_t buffer[64];
                    GetWindowTextW(editBox, buffer, 64);
                    int newValue = _wtoi(buffer);
                    if (newValue >= 0) {
                        tab->detector->setCounter(newValue);
                        SetWindowTextW(m_staticCounter, std::to_wstring(newValue).c_str());
                        updateTabControl();
                        saveCounterToFile(tab);
                    }
                    result = IDOK;
                    done = true;
                    break;
                } else if (msg.hwnd == btnCancel) {
                    result = IDCANCEL;
                    done = true;
                    break;
                }
            }
            
            // Gérer la fermeture via le X
            if (msg.message == WM_CLOSE || msg.message == WM_QUIT) {
                done = true;
                break;
            }
            
            // Gérer le clic sur le bouton X de la fenêtre
            if (msg.message == WM_NCLBUTTONDOWN && msg.hwnd == hwndDialog && msg.wParam == HTCLOSE) {
                done = true;
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(m_hwnd, TRUE);
    if (IsWindow(hwndDialog)) {
        DestroyWindow(hwndDialog);
    }
    SetForegroundWindow(m_hwnd);
}

// ============================================================================
// Sauvegarde du compteur
// ============================================================================

std::wstring MainWindow::getDefaultSavePath(const std::wstring& tabName) {
    // Obtenir le dossier Documents de l'utilisateur
    wchar_t documentsPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, documentsPath))) {
        std::wstring path = documentsPath;
        path += L"\\ImageCounter";
        
        // Créer le dossier s'il n'existe pas
        CreateDirectoryW(path.c_str(), nullptr);
        
        // Nettoyer le nom du tab pour le nom de fichier
        std::wstring cleanName = tabName;
        for (wchar_t& c : cleanName) {
            if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || 
                c == L'?' || c == L'"' || c == L'<' || c == L'>' || c == L'|') {
                c = L'_';
            }
        }
        
        path += L"\\" + cleanName + L".txt";
        return path;
    }
    
    // Fallback: même dossier que l'exécutable
    return tabName + L".txt";
}

void MainWindow::saveCounterToFile(CounterTab* tab) {
    if (!tab) return;
    
    // Utiliser le chemin par défaut si non défini
    if (tab->saveFilePath.empty()) {
        tab->saveFilePath = getDefaultSavePath(tab->name);
    }
    
    // Ouvrir le fichier en écriture
    std::wofstream file(tab->saveFilePath);
    if (file.is_open()) {
        file << tab->detector->getCounter();
        file.close();
    }
}

void MainWindow::loadCounterFromFile(CounterTab* tab) {
    if (!tab) return;
    
    // Utiliser le chemin par défaut si non défini
    if (tab->saveFilePath.empty()) {
        tab->saveFilePath = getDefaultSavePath(tab->name);
    }
    
    // Ouvrir le fichier en lecture
    std::wifstream file(tab->saveFilePath);
    if (file.is_open()) {
        int counter = 0;
        file >> counter;
        file.close();
        
        if (counter > 0) {
            tab->detector->setCounter(counter);
        }
    }
}

// ============================================================================
// Sauvegarde/restauration des paramètres
// ============================================================================

std::wstring MainWindow::getSettingsFilePath() {
    wchar_t documentsPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, documentsPath))) {
        std::wstring path = documentsPath;
        path += L"\\ImageCounter";
        CreateDirectoryW(path.c_str(), nullptr);
        return path + L"\\settings.ini";
    }
    return L"settings.ini";
}

void MainWindow::saveAllSettings() {
    std::wstring settingsPath = getSettingsFilePath();
    
    // Sauvegarder le fichier principal
    std::wofstream file(settingsPath);
    if (!file.is_open()) return;
    
    // En-tête
    file << L"[General]" << std::endl;
    file << L"TabCount=" << m_tabs.size() << std::endl;
    file << L"CurrentTab=" << m_currentTabIndex << std::endl;
    file << std::endl;
    
    // Apparence
    file << L"[Appearance]" << std::endl;
    file << L"Theme=" << (int)ThemeManager::getInstance().getThemeMode() << std::endl;
    file << L"Language=" << (int)Localization::getInstance().getLanguage() << std::endl;
    file << std::endl;
    
    file.close();
    
    // Sauvegarder chaque onglet
    for (size_t i = 0; i < m_tabs.size(); i++) {
        std::wstring tabPath = getSettingsFilePath();
        tabPath = tabPath.substr(0, tabPath.length() - 12); // Enlever "settings.ini"
        tabPath += L"tab_" + std::to_wstring(i) + L".ini";
        saveTabSettings(m_tabs[i].get(), tabPath);
    }
}

void MainWindow::saveTabSettings(CounterTab* tab, const std::wstring& path) {
    if (!tab) return;
    
    std::wofstream file(path);
    if (!file.is_open()) return;
    
    // Sauvegarder l'image de référence si elle existe
    std::wstring refImagePath;
    if (tab->detector->hasReferenceImage()) {
        // Générer le chemin pour l'image de référence
        std::wstring basePath = path.substr(0, path.length() - 4); // Enlever ".ini"
        refImagePath = basePath + L"_ref.png";
        
        // Sauvegarder l'image avec OpenCV
        cv::Mat refImage = tab->detector->getReferenceImage();
        if (!refImage.empty()) {
            // Convertir wstring en string pour OpenCV
            std::string pathStr(refImagePath.begin(), refImagePath.end());
            cv::imwrite(pathStr, refImage);
        }
    }
    
    // Informations de base
    file << L"[Tab]" << std::endl;
    file << L"Name=" << tab->name << std::endl;
    file << L"TargetWindowTitle=" << tab->targetWindowTitle << std::endl;
    file << L"ReferenceImagePath=" << refImagePath << std::endl;
    file << std::endl;
    
    // Configuration de détection
    file << L"[Detection]" << std::endl;
    file << L"Threshold=" << tab->config.threshold << std::endl;
    file << L"ScanIntervalMs=" << tab->config.scanIntervalMs << std::endl;
    file << L"CooldownMs=" << tab->config.cooldownMs << std::endl;
    file << L"DetectMultiple=" << (tab->config.detectMultiple ? 1 : 0) << std::endl;
    file << L"CounterStep=" << tab->config.counterStep << std::endl;
    file << L"UseGrayscale=" << (tab->config.useGrayscale ? 1 : 0) << std::endl;
    file << L"MatchMethod=" << tab->config.matchMethod << std::endl;
    file << std::endl;
    
    // Zone de capture
    file << L"[Region]" << std::endl;
    file << L"UseFullWindow=" << (tab->region.useFullWindow ? 1 : 0) << std::endl;
    file << L"X=" << tab->region.x << std::endl;
    file << L"Y=" << tab->region.y << std::endl;
    file << L"Width=" << tab->region.width << std::endl;
    file << L"Height=" << tab->region.height << std::endl;
    file << std::endl;
    
    // Son
    file << L"[Sound]" << std::endl;
    file << L"Enabled=" << (tab->soundEnabled ? 1 : 0) << std::endl;
    file << L"Index=" << tab->soundIndex << std::endl;
    file << std::endl;
    
    // Sauvegarde
    file << L"[Save]" << std::endl;
    file << L"FilePath=" << tab->saveFilePath << std::endl;
    file << std::endl;
    
    // Raccourci clavier
    file << L"[Hotkey]" << std::endl;
    file << L"VirtualKey=" << tab->hotkeyVirtualKey << std::endl;
    file << L"Modifiers=" << tab->hotkeyModifiers << std::endl;
    
    file.close();
}

void MainWindow::loadAllSettings() {
    std::wstring settingsPath = getSettingsFilePath();
    
    std::wifstream file(settingsPath);
    if (!file.is_open()) return;
    
    int tabCount = 0;
    int currentTab = 0;
    int theme = 0;
    int language = 0;
    std::wstring section;
    
    std::wstring line;
    while (std::getline(file, line)) {
        // Supprimer les espaces
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n' || line.back() == L' ')) {
            line.pop_back();
        }
        
        if (line.empty()) continue;
        
        // Section
        if (line.front() == L'[' && line.back() == L']') {
            section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Clé=Valeur
        size_t pos = line.find(L'=');
        if (pos == std::wstring::npos) continue;
        
        std::wstring key = line.substr(0, pos);
        std::wstring value = line.substr(pos + 1);
        
        if (section == L"General") {
            if (key == L"TabCount") tabCount = std::stoi(value);
            else if (key == L"CurrentTab") currentTab = std::stoi(value);
        }
        else if (section == L"Appearance") {
            if (key == L"Theme") theme = std::stoi(value);
            else if (key == L"Language") language = std::stoi(value);
        }
    }
    file.close();
    
    // Appliquer le thème et la langue
    ThemeManager::getInstance().setThemeMode((ThemeMode)theme);
    Localization::getInstance().setLanguage((Language)language);
    
    // Rafraîchir l'UI
    ThemeManager::getInstance().applyToWindow(m_hwnd);
    refreshUILanguage();
    
    if (tabCount <= 0) return;
    
    // Supprimer les onglets par défaut
    m_tabs.clear();
    TabCtrl_DeleteAllItems(m_tabControl);
    
    // Charger chaque onglet
    for (int i = 0; i < tabCount; i++) {
        auto tab = std::make_unique<CounterTab>();
        
        std::wstring tabPath = settingsPath.substr(0, settingsPath.length() - 12);
        tabPath += L"tab_" + std::to_wstring(i) + L".ini";
        
        loadTabSettings(tab.get(), tabPath);
        
        // Configurer le callback
        int tabIndex = (int)m_tabs.size();
        tab->detector->setDetectionCallback([this, tabIndex](const DetectionResult& result, int counter) {
            onDetection(tabIndex, result, counter);
        });
        
        // Charger le compteur
        loadCounterFromFile(tab.get());
        
        // Charger l'image de référence si elle existe
        if (!tab->referenceImagePath.empty()) {
            if (tab->detector->loadReferenceImage(tab->referenceImagePath)) {
                // Mettre à jour la prévisualisation
                updatePreviewForTab(tab.get(), tab->detector->getReferenceImage());
            }
        }
        
        m_tabs.push_back(std::move(tab));
        
        // Ajouter l'onglet au contrôle
        TCITEMW tie = {};
        tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<LPWSTR>(m_tabs.back()->name.c_str());
        TabCtrl_InsertItem(m_tabControl, (int)m_tabs.size() - 1, &tie);
    }
    
    // Restaurer la fenêtre cible pour chaque onglet
    updateWindowList();
    for (auto& tab : m_tabs) {
        if (!tab->targetWindowTitle.empty()) {
            for (size_t i = 0; i < m_windowList.size(); i++) {
                if (m_windowList[i].second == tab->targetWindowTitle) {
                    tab->selectedWindowIndex = (int)i;
                    tab->targetWindow = m_windowList[i].first;
                    tab->detector->setTargetWindow(m_windowList[i].first);
                    break;
                }
            }
        }
    }
    
    // Sélectionner l'onglet actif
    if (currentTab >= 0 && currentTab < (int)m_tabs.size()) {
        TabCtrl_SetCurSel(m_tabControl, currentTab);
        switchToTab(currentTab);
    }
    
    // Enregistrer tous les raccourcis clavier
    registerHotkeys();
}

void MainWindow::loadTabSettings(CounterTab* tab, const std::wstring& path) {
    if (!tab) return;
    
    std::wifstream file(path);
    if (!file.is_open()) {
        tab->name = L"Compteur";
        return;
    }
    
    std::wstring line;
    std::wstring section;
    
    while (std::getline(file, line)) {
        // Supprimer les espaces
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n' || line.back() == L' ')) {
            line.pop_back();
        }
        
        if (line.empty()) continue;
        
        // Section
        if (line.front() == L'[' && line.back() == L']') {
            section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Clé=Valeur
        size_t pos = line.find(L'=');
        if (pos == std::wstring::npos) continue;
        
        std::wstring key = line.substr(0, pos);
        std::wstring value = line.substr(pos + 1);
        
        if (section == L"Tab") {
            if (key == L"Name") tab->name = value;
            else if (key == L"TargetWindowTitle") tab->targetWindowTitle = value;
            else if (key == L"ReferenceImagePath") tab->referenceImagePath = value;
        }
        else if (section == L"Detection") {
            if (key == L"Threshold") tab->config.threshold = std::stod(value);
            else if (key == L"ScanIntervalMs") tab->config.scanIntervalMs = std::stoi(value);
            else if (key == L"CooldownMs") tab->config.cooldownMs = std::stoi(value);
            else if (key == L"DetectMultiple") tab->config.detectMultiple = (std::stoi(value) != 0);
            else if (key == L"CounterStep") tab->config.counterStep = std::stoi(value);
            else if (key == L"UseGrayscale") tab->config.useGrayscale = (std::stoi(value) != 0);
            else if (key == L"MatchMethod") tab->config.matchMethod = std::stoi(value);
        }
        else if (section == L"Region") {
            if (key == L"UseFullWindow") tab->region.useFullWindow = (std::stoi(value) != 0);
            else if (key == L"X") tab->region.x = std::stoi(value);
            else if (key == L"Y") tab->region.y = std::stoi(value);
            else if (key == L"Width") tab->region.width = std::stoi(value);
            else if (key == L"Height") tab->region.height = std::stoi(value);
        }
        else if (section == L"Sound") {
            if (key == L"Enabled") tab->soundEnabled = (std::stoi(value) != 0);
            else if (key == L"Index") tab->soundIndex = std::stoi(value);
        }
        else if (section == L"Save") {
            if (key == L"FilePath") tab->saveFilePath = value;
        }
        else if (section == L"Hotkey") {
            if (key == L"VirtualKey") tab->hotkeyVirtualKey = std::stoi(value);
            else if (key == L"Modifiers") tab->hotkeyModifiers = std::stoi(value);
        }
    }
    
    file.close();
    
    // Appliquer la configuration au détecteur
    tab->detector->setConfig(tab->config);
    tab->detector->setCaptureRegion(tab->region);
}

// ============================================================================
// Langue et thème
// ============================================================================

void MainWindow::refreshUILanguage() {
    // Mettre à jour le titre de la fenêtre
    SetWindowTextW(m_hwnd, TR("app_title"));
    
    // Mettre à jour les boutons
    SetWindowTextW(m_btnLoadImage, TR("load_image"));
    SetWindowTextW(m_btnQuickCapture, TR("quick_capture"));
    SetWindowTextW(m_btnSelectRegion, TR("select_zone"));
    SetWindowTextW(m_btnSettings, TR("settings"));
    SetWindowTextW(m_btnStart, TR("start"));
    SetWindowTextW(m_btnStop, TR("stop"));
    SetWindowTextW(m_btnReset, TR("reset"));
    
    // Mettre à jour les labels
    SetWindowTextW(m_labelTargetWindow, TR("target_window"));
    SetWindowTextW(m_labelCounter, TR("counter_label"));
    SetWindowTextW(m_labelRefImage, TR("reference_image"));
    SetWindowTextW(m_labelCapture, TR("current_capture"));
    
    // Forcer le repaint
    InvalidateRect(m_hwnd, nullptr, TRUE);
    
    // Mettre à jour l'UI
    updateUIForCurrentTab();
}

void MainWindow::applyTheme() {
    // Supprimer l'ancien brush
    if (m_backgroundBrush) {
        DeleteObject(m_backgroundBrush);
        m_backgroundBrush = nullptr;
    }
    
    // Appliquer le thème à la barre de titre
    ThemeManager::getInstance().applyToWindow(m_hwnd);
    
    // Appliquer le thème sombre aux contrôles
    bool isDark = ThemeManager::getInstance().isDarkTheme();
    const wchar_t* theme = isDark ? L"DarkMode_Explorer" : nullptr;
    const wchar_t* cfdTheme = isDark ? L"DarkMode_CFD" : nullptr;
    
    // Appliquer aux boutons
    SetWindowTheme(m_btnLoadImage, theme, nullptr);
    SetWindowTheme(m_btnQuickCapture, theme, nullptr);
    SetWindowTheme(m_btnSelectRegion, theme, nullptr);
    SetWindowTheme(m_btnSettings, theme, nullptr);
    SetWindowTheme(m_btnHistory, theme, nullptr);
    SetWindowTheme(m_btnStart, theme, nullptr);
    SetWindowTheme(m_btnStop, theme, nullptr);
    SetWindowTheme(m_btnReset, theme, nullptr);
    SetWindowTheme(m_btnRefreshWindows, theme, nullptr);
    SetWindowTheme(m_btnAddTab, theme, nullptr);
    SetWindowTheme(m_btnRemoveTab, theme, nullptr);
    SetWindowTheme(m_btnRenameTab, theme, nullptr);
    SetWindowTheme(m_btnCounterMinus, theme, nullptr);
    SetWindowTheme(m_btnCounterPlus, theme, nullptr);
    
    // Appliquer aux onglets
    SetWindowTheme(m_tabControl, theme, nullptr);
    
    // Appliquer aux combos et edits avec le thème CFD
    SetWindowTheme(m_comboWindows, cfdTheme, nullptr);
    
    // Envoyer le message pour forcer le redessinage des contrôles
    SendMessageW(m_comboWindows, WM_THEMECHANGED, 0, 0);
    SendMessageW(m_tabControl, WM_THEMECHANGED, 0, 0);
    
    // Forcer le repaint de tous les contrôles
    InvalidateRect(m_hwnd, nullptr, TRUE);
    
    // Redessiner tous les enfants avec le bon thème selon leur classe
    struct ThemeData {
        bool isDark;
    } data = { isDark };
    
    EnumChildWindows(m_hwnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* data = reinterpret_cast<ThemeData*>(lParam);
        if (data->isDark) {
            wchar_t className[64] = {0};
            GetClassNameW(hwnd, className, 64);
            if (wcscmp(className, L"ComboBox") == 0 || wcscmp(className, L"Edit") == 0) {
                SetWindowTheme(hwnd, L"DarkMode_CFD", nullptr);
            } else {
                SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
            }
        } else {
            SetWindowTheme(hwnd, nullptr, nullptr);
        }
        InvalidateRect(hwnd, nullptr, TRUE);
        return TRUE;
    }, (LPARAM)&data);
}

// ============================================================================
// Historique des détections
// ============================================================================

void MainWindow::addDetectionToHistory(CounterTab* tab, int counterValue, double confidence) {
    if (!tab) return;
    
    DetectionEntry entry;
    GetLocalTime(&entry.timestamp);
    entry.counterValue = counterValue;
    entry.confidence = confidence;
    entry.windowTitle = tab->targetWindowTitle;
    
    tab->detectionHistory.push_back(entry);
    
    // Limiter l'historique à 10000 entrées pour éviter les problèmes de mémoire
    if (tab->detectionHistory.size() > 10000) {
        tab->detectionHistory.erase(tab->detectionHistory.begin());
    }
}

void MainWindow::clearHistory(CounterTab* tab) {
    if (!tab) return;
    tab->detectionHistory.clear();
}

void MainWindow::exportHistoryToCSV(CounterTab* tab) {
    if (!tab || tab->detectionHistory.empty()) {
        MessageBoxW(m_hwnd, TR("history_empty"), TR("history"), MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Dialogue de sauvegarde
    wchar_t szFile[MAX_PATH] = L"historique.csv";
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"CSV (*.csv)\0*.csv\0Tous les fichiers (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"csv";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    
    if (!GetSaveFileNameW(&ofn)) {
        return;
    }
    
    // Écrire le fichier CSV
    std::wofstream file(szFile);
    if (!file.is_open()) {
        MessageBoxW(m_hwnd, TR("error_save_failed"), TR("error"), MB_OK | MB_ICONERROR);
        return;
    }
    
    // En-tête CSV
    file << L"Date;Heure;Compteur;Confiance;Fenêtre" << std::endl;
    
    // Données
    for (const auto& entry : tab->detectionHistory) {
        wchar_t dateStr[32], timeStr[32];
        swprintf_s(dateStr, L"%04d-%02d-%02d", 
            entry.timestamp.wYear, entry.timestamp.wMonth, entry.timestamp.wDay);
        swprintf_s(timeStr, L"%02d:%02d:%02d", 
            entry.timestamp.wHour, entry.timestamp.wMinute, entry.timestamp.wSecond);
        
        file << dateStr << L";" 
             << timeStr << L";"
             << entry.counterValue << L";"
             << (int)(entry.confidence * 100) << L"%;"
             << entry.windowTitle << std::endl;
    }
    
    file.close();
    
    // Message de confirmation
    std::wstring msg = Localization::getInstance().getLanguage() == Language::French ?
        L"Historique exporté avec succès !\n" + std::to_wstring(tab->detectionHistory.size()) + L" entrées." :
        L"History exported successfully!\n" + std::to_wstring(tab->detectionHistory.size()) + L" entries.";
    MessageBoxW(m_hwnd, msg.c_str(), TR("history"), MB_OK | MB_ICONINFORMATION);
}

void MainWindow::showHistory() {
    auto* tab = currentTab();
    if (!tab) return;
    
    // Créer la fenêtre de dialogue pour l'historique
    const wchar_t* className = L"HistoryDialogClass";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = ThemeManager::getInstance().isDarkTheme() ? 
        CreateSolidBrush(RGB(32, 32, 32)) : (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);
    
    RECT parentRect;
    GetWindowRect(m_hwnd, &parentRect);
    int w = 800, h = 550;
    int x = parentRect.left + (parentRect.right - parentRect.left - w) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - h) / 2;
    
    HWND hwndDialog = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        TR("history"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, w, h,
        m_hwnd, nullptr, m_hInstance, nullptr);
    
    if (!hwndDialog) return;
    
    bool isDark = ThemeManager::getInstance().isDarkTheme();
    
    // Obtenir la zone client
    RECT clientRect;
    GetClientRect(hwndDialog, &clientRect);
    int clientW = clientRect.right;
    int clientH = clientRect.bottom;
    
    // Créer la ListView pour afficher l'historique
    HWND listView = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        10, 10, clientW - 20, clientH - 90,
        hwndDialog, (HMENU)1001, m_hInstance, nullptr);
    
    // Style étendu pour la ListView
    ListView_SetExtendedListViewStyle(listView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    
    // Colonnes (largeurs ajustées pour DPI élevé)
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    
    col.cx = 110;
    col.pszText = (LPWSTR)L"Date";
    col.iSubItem = 0;
    ListView_InsertColumn(listView, 0, &col);
    
    col.cx = 90;
    col.pszText = (LPWSTR)L"Heure";
    col.iSubItem = 1;
    ListView_InsertColumn(listView, 1, &col);
    
    col.cx = 100;
    col.pszText = (LPWSTR)L"Compteur";
    col.iSubItem = 2;
    ListView_InsertColumn(listView, 2, &col);
    
    col.cx = 100;
    col.pszText = (LPWSTR)L"Confiance";
    col.iSubItem = 3;
    ListView_InsertColumn(listView, 3, &col);
    
    col.cx = 350;
    col.pszText = (LPWSTR)L"Fenêtre";
    col.iSubItem = 4;
    ListView_InsertColumn(listView, 4, &col);
    
    // Remplir la ListView (du plus récent au plus ancien)
    for (int i = (int)tab->detectionHistory.size() - 1; i >= 0; i--) {
        const auto& entry = tab->detectionHistory[i];
        
        wchar_t dateStr[32], timeStr[32], counterStr[32], confStr[32];
        swprintf_s(dateStr, L"%04d-%02d-%02d", 
            entry.timestamp.wYear, entry.timestamp.wMonth, entry.timestamp.wDay);
        swprintf_s(timeStr, L"%02d:%02d:%02d", 
            entry.timestamp.wHour, entry.timestamp.wMinute, entry.timestamp.wSecond);
        swprintf_s(counterStr, L"%d", entry.counterValue);
        swprintf_s(confStr, L"%d%%", (int)(entry.confidence * 100));
        
        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = (int)tab->detectionHistory.size() - 1 - i;
        item.pszText = dateStr;
        int idx = ListView_InsertItem(listView, &item);
        
        ListView_SetItemText(listView, idx, 1, timeStr);
        ListView_SetItemText(listView, idx, 2, counterStr);
        ListView_SetItemText(listView, idx, 3, confStr);
        ListView_SetItemText(listView, idx, 4, (LPWSTR)entry.windowTitle.c_str());
    }
    
    // Appliquer le thème sombre à la ListView
    if (isDark) {
        ListView_SetBkColor(listView, RGB(32, 32, 32));
        ListView_SetTextBkColor(listView, RGB(32, 32, 32));
        ListView_SetTextColor(listView, RGB(255, 255, 255));
        SetWindowTheme(listView, L"DarkMode_Explorer", nullptr);
    }
    
    // Info sur le nombre d'entrées
    std::wstring infoText = std::to_wstring(tab->detectionHistory.size()) + L" " + 
        (Localization::getInstance().getLanguage() == Language::French ? L"détection(s)" : L"detection(s)");
    HWND labelInfo = CreateWindowExW(0, L"STATIC", infoText.c_str(),
        WS_CHILD | WS_VISIBLE,
        10, clientH - 70, 200, 20,
        hwndDialog, nullptr, m_hInstance, nullptr);
    
    // Boutons
    HWND btnExport = CreateWindowExW(0, L"BUTTON", TR("export_csv"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, clientH - 45, 120, 30,
        hwndDialog, (HMENU)IDOK, m_hInstance, nullptr);
    
    HWND btnClear = CreateWindowExW(0, L"BUTTON", TR("clear_history"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        140, clientH - 45, 120, 30,
        hwndDialog, (HMENU)IDABORT, m_hInstance, nullptr);
    
    HWND btnClose = CreateWindowExW(0, L"BUTTON", TR("close"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        clientW - 130, clientH - 45, 110, 30,
        hwndDialog, (HMENU)IDCANCEL, m_hInstance, nullptr);
    
    // Appliquer le thème aux boutons
    if (isDark) {
        SetWindowTheme(btnExport, L"DarkMode_Explorer", nullptr);
        SetWindowTheme(btnClear, L"DarkMode_Explorer", nullptr);
        SetWindowTheme(btnClose, L"DarkMode_Explorer", nullptr);
    }
    
    EnableWindow(m_hwnd, FALSE);
    ShowWindow(hwndDialog, SW_SHOW);
    UpdateWindow(hwndDialog);
    
    // Boucle de messages
    MSG msg = {};
    bool running = true;
    
    while (running) {
        DWORD waitResult = MsgWaitForMultipleObjects(0, nullptr, FALSE, 100, QS_ALLINPUT);
        
        if (waitResult == WAIT_TIMEOUT) {
            if (!IsWindow(hwndDialog)) {
                running = false;
            }
            continue;
        }
        
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (!IsWindow(hwndDialog)) {
                running = false;
                break;
            }
            
            if (msg.message == WM_QUIT) {
                running = false;
                PostQuitMessage((int)msg.wParam);
                break;
            }
            
            // Gérer les clics sur les boutons
            if (msg.message == WM_LBUTTONUP) {
                if (msg.hwnd == btnExport) {
                    exportHistoryToCSV(tab);
                    continue;
                } else if (msg.hwnd == btnClear) {
                    int confirm = MessageBoxW(hwndDialog, 
                        TR("confirm_clear_history"), 
                        TR("confirmation"), 
                        MB_YESNO | MB_ICONQUESTION);
                    if (confirm == IDYES) {
                        clearHistory(tab);
                        ListView_DeleteAllItems(listView);
                        SetWindowTextW(labelInfo, L"0 détection(s)");
                    }
                    continue;
                } else if (msg.hwnd == btnClose) {
                    running = false;
                    break;
                }
            }
            
            // Gérer Échap
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                running = false;
                break;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    EnableWindow(m_hwnd, TRUE);
    if (IsWindow(hwndDialog)) {
        DestroyWindow(hwndDialog);
    }
    SetForegroundWindow(m_hwnd);
}

// ============================================================================
// Raccourcis clavier globaux
// ============================================================================

void MainWindow::registerHotkeys() {
    // Désenregistrer tous les anciens raccourcis
    unregisterHotkeys();
    
    // Enregistrer les raccourcis de tous les onglets
    for (size_t i = 0; i < m_tabs.size(); i++) {
        auto& tab = m_tabs[i];
        if (tab->hotkeyVirtualKey != 0) {
            UINT modifiers = 0;
            if (tab->hotkeyModifiers & MOD_CONTROL) modifiers |= MOD_CONTROL;
            if (tab->hotkeyModifiers & MOD_SHIFT) modifiers |= MOD_SHIFT;
            if (tab->hotkeyModifiers & MOD_ALT) modifiers |= MOD_ALT;
            
            // ID unique par onglet : HOTKEY_INCREMENT + index
            RegisterHotKey(m_hwnd, HOTKEY_INCREMENT + (int)i, modifiers, tab->hotkeyVirtualKey);
        }
    }
}

void MainWindow::unregisterHotkeys() {
    // Désenregistrer tous les hotkeys possibles (max 100 onglets)
    for (int i = 0; i < 100; i++) {
        UnregisterHotKey(m_hwnd, HOTKEY_INCREMENT + i);
    }
}

void MainWindow::onHotkey(WPARAM wParam) {
    int tabIndex = (int)wParam - HOTKEY_INCREMENT;
    if (tabIndex >= 0 && tabIndex < (int)m_tabs.size()) {
        incrementCounterForTab(tabIndex);
    }
}

void MainWindow::incrementCounter() {
    incrementCounterForTab(m_currentTabIndex);
}

void MainWindow::incrementCounterForTab(int tabIndex) {
    if (tabIndex < 0 || tabIndex >= (int)m_tabs.size()) return;
    
    auto& tab = m_tabs[tabIndex];
    
    int step = tab->config.counterStep;
    int counter = tab->detector->getCounter() + step;
    tab->detector->setCounter(counter);
    
    // Mettre à jour l'affichage si c'est l'onglet actif
    if (tabIndex == m_currentTabIndex) {
        SetWindowTextW(m_staticCounter, std::to_wstring(counter).c_str());
    }
    
    updateTabControl();
    saveCounterToFile(tab.get());
    
    // Ajouter à l'historique
    addDetectionToHistory(tab.get(), counter, 1.0);  // 100% confiance pour les ajouts manuels
    
    // Jouer le son si activé
    if (tab->soundEnabled) {
        switch (tab->soundIndex) {
            case 0: MessageBeep(MB_OK); break;
            case 1: MessageBeep(MB_ICONASTERISK); break;
            case 2: PlaySoundW(L"SystemNotification", nullptr, SND_ALIAS | SND_ASYNC); break;
        }
    }
}
