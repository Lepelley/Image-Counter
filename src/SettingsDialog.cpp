#include "SettingsDialog.h"
#include <commctrl.h>
#include <commdlg.h>
#include <uxtheme.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")

using namespace SettingsIDs;

// ============================================================================
// SettingsDialog Implementation
// ============================================================================

SettingsDialog::SettingsDialog(HWND parent, ExtendedConfig& config, HWND targetWindow)
    : m_parent(parent)
    , m_config(config)
    , m_originalConfig(config)
    , m_targetWindow(targetWindow)
{
    m_hInstance = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
}

SettingsDialog::~SettingsDialog() {}

bool SettingsDialog::show() {
    // Créer la fenêtre de dialogue manuellement
    const wchar_t* className = L"ImageCounterSettingsDialog";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DialogProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = className;
    
    wc.hbrBackground = ThemeManager::getInstance().isDarkTheme() ?
        CreateSolidBrush(RGB(32, 32, 32)) : (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClassExW(&wc);
    
    // Centrer la fenêtre sur le parent
    RECT parentRect;
    GetWindowRect(m_parent, &parentRect);
    int dialogWidth = 450;
    int dialogHeight = 465;
    int x = parentRect.left + (parentRect.right - parentRect.left - dialogWidth) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - dialogHeight) / 2;
    
    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        className,
        TR("settings_title"),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, dialogWidth, dialogHeight,
        m_parent,
        nullptr,
        m_hInstance,
        this
    );
    
    if (!m_hwnd) {
        return false;
    }
    
    // Désactiver la fenêtre parente
    EnableWindow(m_parent, FALSE);
    
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    // Boucle de messages modale
    m_dialogResult = IDCANCEL;
    MSG msg;
    while (IsWindow(m_hwnd) && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Réactiver la fenêtre parente
    EnableWindow(m_parent, TRUE);
    SetForegroundWindow(m_parent);
    
    return (m_dialogResult == IDOK);
}

INT_PTR CALLBACK SettingsDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsDialog* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<SettingsDialog*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<SettingsDialog*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        switch (msg) {
            case WM_CREATE:
                pThis->onInitDialog(hwnd);
                return 0;
                
            case WM_COMMAND:
                pThis->onCommand(hwnd, wParam);
                return 0;
                
            case WM_NOTIFY:
                pThis->onNotify(hwnd, lParam);
                return 0;
                
            case WM_HSCROLL:
                pThis->onHScroll(hwnd, wParam, lParam);
                return 0;
            
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            case WM_CTLCOLORSTATIC: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkMode(hdc, TRANSPARENT);
                    if (!pThis->m_darkBrush) pThis->m_darkBrush = CreateSolidBrush(RGB(32, 32, 32));
                    return (INT_PTR)pThis->m_darkBrush;
                }
                break;
            }
            
            case WM_CTLCOLORBTN: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkMode(hdc, TRANSPARENT);
                    if (!pThis->m_darkBrush) pThis->m_darkBrush = CreateSolidBrush(RGB(32, 32, 32));
                    return (INT_PTR)pThis->m_darkBrush;
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
            
            case WM_CTLCOLORLISTBOX: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(45, 45, 45));
                    static HBRUSH listBrush = CreateSolidBrush(RGB(45, 45, 45));
                    return (INT_PTR)listBrush;
                }
                break;
            }
            
            case WM_ERASEBKGND: {
                if (ThemeManager::getInstance().isDarkTheme()) {
                    HDC hdc = (HDC)wParam;
                    RECT rect;
                    GetClientRect(hwnd, &rect);
                    if (!pThis->m_darkBrush) pThis->m_darkBrush = CreateSolidBrush(RGB(32, 32, 32));
                    FillRect(hdc, &rect, pThis->m_darkBrush);
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
                
            case WM_CLOSE:
                pThis->m_dialogResult = IDCANCEL;
                DestroyWindow(hwnd);
                return 0;
                
            case WM_DESTROY:
                if (pThis->m_darkBrush) {
                    DeleteObject(pThis->m_darkBrush);
                    pThis->m_darkBrush = nullptr;
                }
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SettingsDialog::onInitDialog(HWND hwnd) {
    m_hwnd = hwnd;
    
    // Mettre à jour le titre
    SetWindowTextW(hwnd, TR("settings_title"));
    
    // Appliquer le thème sombre à la barre de titre
    ThemeManager::getInstance().applyToWindow(hwnd);
    
    // Créer le contrôle d'onglets
    createTabControl(hwnd);
    
    // Créer les contrôles de chaque onglet
    createDetectionTab(hwnd);
    createRegionTab(hwnd);
    createCounterTab(hwnd);
    createAdvancedTab(hwnd);
    
    // Boutons OK, Annuler, Réinitialiser
   int btnY = 390;
    int btnWidth = 95;
    int btnHeight = 28;
    
    HWND btnDefault = CreateWindowExW(0, L"BUTTON", TR("default"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, btnY, btnWidth, btnHeight,
        hwnd, (HMENU)IDC_BTN_RESET_DEFAULTS, m_hInstance, nullptr);
    
    HWND btnOK = CreateWindowExW(0, L"BUTTON", TR("ok"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        220, btnY, btnWidth, btnHeight,
        hwnd, (HMENU)IDC_BTN_OK, m_hInstance, nullptr);
    
    HWND btnCancel = CreateWindowExW(0, L"BUTTON", TR("cancel"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        325, btnY, btnWidth, btnHeight,
        hwnd, (HMENU)IDC_BTN_CANCEL, m_hInstance, nullptr);
    
    // Appliquer le thème sombre aux contrôles
    if (ThemeManager::getInstance().isDarkTheme()) {
        const wchar_t* explorerTheme = L"DarkMode_Explorer";
        const wchar_t* cfdTheme = L"DarkMode_CFD";
        
        SetWindowTheme(btnOK, explorerTheme, nullptr);
        SetWindowTheme(btnCancel, explorerTheme, nullptr);
        SetWindowTheme(btnDefault, explorerTheme, nullptr);
        SetWindowTheme(m_tabControl, explorerTheme, nullptr);
        
        // Appliquer aux contrôles de chaque onglet
        auto applyThemeToControl = [&](HWND ctrl) {
            wchar_t className[64] = {0};
            GetClassNameW(ctrl, className, 64);
            
            LONG style = GetWindowLong(ctrl, GWL_STYLE);
            bool isRadioOrCheck = (wcscmp(className, L"Button") == 0) && 
                ((style & BS_AUTORADIOBUTTON) || (style & BS_AUTOCHECKBOX) || 
                 (style & BS_RADIOBUTTON) || (style & BS_CHECKBOX));
            
            if (wcscmp(className, L"ComboBox") == 0 || wcscmp(className, L"Edit") == 0) {
                SetWindowTheme(ctrl, cfdTheme, nullptr);
            } else if (isRadioOrCheck) {
                // Pour les radio buttons et checkboxes, utiliser un thème spécial
                SetWindowTheme(ctrl, L"", L"");
            } else {
                SetWindowTheme(ctrl, explorerTheme, nullptr);
            }
        };
        
        for (HWND ctrl : m_detectionControls) applyThemeToControl(ctrl);
        for (HWND ctrl : m_regionControls) applyThemeToControl(ctrl);
        for (HWND ctrl : m_counterControls) applyThemeToControl(ctrl);
        for (HWND ctrl : m_advancedControls) applyThemeToControl(ctrl);
    }
    
    // Charger la configuration actuelle dans l'UI
    loadConfigToUI(hwnd);
    
    // Afficher le premier onglet
    showTabPage(0);
}

void SettingsDialog::createTabControl(HWND hwnd) {
    m_tabControl = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_OWNERDRAWFIXED,
        10, 10, 420, 345,
        hwnd, (HMENU)IDC_TAB_CONTROL, m_hInstance, nullptr);
    
    // Subclasser le Tab Control pour dessiner le fond sombre
    if (ThemeManager::getInstance().isDarkTheme()) {
        SetWindowSubclass(m_tabControl, [](HWND hWnd, UINT uMsg, WPARAM wParam, 
            LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT {
            
            if (uMsg == WM_ERASEBKGND) {
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
    }
    
    // Ajouter les onglets (Compteur en premier)
    TCITEMW tie = {};
    tie.mask = TCIF_TEXT;
    
    tie.pszText = const_cast<LPWSTR>(TR("tab_counter"));
    TabCtrl_InsertItem(m_tabControl, 0, &tie);
    
    tie.pszText = const_cast<LPWSTR>(TR("tab_detection"));
    TabCtrl_InsertItem(m_tabControl, 1, &tie);
    
    tie.pszText = const_cast<LPWSTR>(TR("tab_region"));
    TabCtrl_InsertItem(m_tabControl, 2, &tie);
    
    tie.pszText = const_cast<LPWSTR>(TR("tab_advanced"));
    TabCtrl_InsertItem(m_tabControl, 3, &tie);
    
    // Sélectionner l'onglet initial
    TabCtrl_SetCurSel(m_tabControl, m_initialTab);
    showTabPage(m_initialTab);
}

void SettingsDialog::createDetectionTab(HWND hwnd) {
    int baseY = 45;
    int leftMargin = 25;
    int labelWidth = 180;
    int controlWidth = 170;
    int rowHeight = 35;
    
    // Seuil de détection
    HWND label1 = CreateWindowExW(0, L"STATIC", TR("threshold"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY, labelWidth, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_detectionControls.push_back(label1);
    
    HWND slider = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        leftMargin + labelWidth, baseY - 5, controlWidth, 30,
        hwnd, (HMENU)IDC_SLIDER_THRESHOLD, m_hInstance, nullptr);
    SendMessage(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
    SendMessage(slider, TBM_SETTICFREQ, 10, 0);
    m_detectionControls.push_back(slider);
    
    HWND thresholdValue = CreateWindowExW(0, L"STATIC", L"85%",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        leftMargin + labelWidth + controlWidth + 10, baseY, 40, 20,
        hwnd, (HMENU)IDC_STATIC_THRESHOLD_VALUE, m_hInstance, nullptr);
    m_detectionControls.push_back(thresholdValue);
    
    baseY += rowHeight + 5;
    
    // Intervalle de scan (en secondes)
    HWND label2 = CreateWindowExW(0, L"STATIC", TR("scan_interval"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, labelWidth, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_detectionControls.push_back(label2);
    
    HWND editScan = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0.5",
        WS_CHILD | WS_VISIBLE | ES_RIGHT,
        leftMargin + labelWidth, baseY, 80, 24,
        hwnd, (HMENU)IDC_EDIT_SCAN_INTERVAL, m_hInstance, nullptr);
    m_detectionControls.push_back(editScan);
    
    HWND labelMs1 = CreateWindowExW(0, L"STATIC", TR("seconds_short"),
        WS_CHILD | WS_VISIBLE,
        leftMargin + labelWidth + 85, baseY + 3, 100, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_detectionControls.push_back(labelMs1);
    
    baseY += rowHeight;
    
    // Cooldown (en secondes)
    HWND label3 = CreateWindowExW(0, L"STATIC", TR("cooldown"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, labelWidth, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_detectionControls.push_back(label3);
    
    HWND editCooldown = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"10",
        WS_CHILD | WS_VISIBLE | ES_RIGHT,
        leftMargin + labelWidth, baseY, 80, 24,
        hwnd, (HMENU)IDC_EDIT_COOLDOWN, m_hInstance, nullptr);
    m_detectionControls.push_back(editCooldown);
    
    HWND labelMs2 = CreateWindowExW(0, L"STATIC", TR("seconds_cooldown"),
        WS_CHILD | WS_VISIBLE,
        leftMargin + labelWidth + 85, baseY + 3, 100, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_detectionControls.push_back(labelMs2);
    
    baseY += rowHeight + 10;
    
    // Checkbox détection multiple
    HWND checkMultiple = CreateWindowExW(0, L"BUTTON", TR("detect_multiple"),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        leftMargin, baseY, 280, 24,
        hwnd, (HMENU)IDC_CHECK_DETECT_MULTIPLE, m_hInstance, nullptr);
    m_detectionControls.push_back(checkMultiple);
    
    baseY += rowHeight;
    
    // Info
    HWND info = CreateWindowExW(0, L"STATIC", TR("detection_info"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY, 380, 40,
        hwnd, nullptr, m_hInstance, nullptr);
    m_detectionControls.push_back(info);
}

void SettingsDialog::createRegionTab(HWND hwnd) {
    int baseY = 45;
    int leftMargin = 25;
    
    // Radio buttons
    HWND radioFull = CreateWindowExW(0, L"BUTTON", TR("full_window"),
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        leftMargin, baseY, 250, 24,
        hwnd, (HMENU)IDC_RADIO_FULL_WINDOW, m_hInstance, nullptr);
    m_regionControls.push_back(radioFull);
    
    baseY += 30;
    
    HWND radioCustom = CreateWindowExW(0, L"BUTTON", TR("custom_region"),
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        leftMargin, baseY, 250, 24,
        hwnd, (HMENU)IDC_RADIO_CUSTOM_REGION, m_hInstance, nullptr);
    m_regionControls.push_back(radioCustom);
    
    baseY += 35;
    
    // Coordonnées de la zone - disposition compacte
    int editWidth = 55;
    
    // Ligne 1: X et Y
    HWND labelX = CreateWindowExW(0, L"STATIC", L"X:",
        WS_CHILD | WS_VISIBLE,
        leftMargin + 20, baseY + 3, 20, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_regionControls.push_back(labelX);
    
    HWND editX = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        leftMargin + 42, baseY, editWidth, 24,
        hwnd, (HMENU)IDC_EDIT_REGION_X, m_hInstance, nullptr);
    m_regionControls.push_back(editX);
    
    HWND labelY = CreateWindowExW(0, L"STATIC", L"Y:",
        WS_CHILD | WS_VISIBLE,
        leftMargin + 110, baseY + 3, 20, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_regionControls.push_back(labelY);
    
    HWND editY = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        leftMargin + 132, baseY, editWidth, 24,
        hwnd, (HMENU)IDC_EDIT_REGION_Y, m_hInstance, nullptr);
    m_regionControls.push_back(editY);
    
    baseY += 32;
    
    // Ligne 2: Largeur (L:) et Hauteur (H:)
    HWND labelW = CreateWindowExW(0, L"STATIC", L"L:",
        WS_CHILD | WS_VISIBLE,
        leftMargin + 20, baseY + 3, 20, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_regionControls.push_back(labelW);
    
    HWND editW = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        leftMargin + 42, baseY, editWidth, 24,
        hwnd, (HMENU)IDC_EDIT_REGION_W, m_hInstance, nullptr);
    m_regionControls.push_back(editW);
    
    HWND labelH = CreateWindowExW(0, L"STATIC", L"H:",
        WS_CHILD | WS_VISIBLE,
        leftMargin + 110, baseY + 3, 20, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_regionControls.push_back(labelH);
    
    HWND editH = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER,
        leftMargin + 132, baseY, editWidth, 24,
        hwnd, (HMENU)IDC_EDIT_REGION_H, m_hInstance, nullptr);
    m_regionControls.push_back(editH);
    
    baseY += 40;
    
    // Bouton sélection visuelle
    HWND btnPick = CreateWindowExW(0, L"BUTTON", TR("pick_region"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin + 20, baseY, 260, 30,
        hwnd, (HMENU)IDC_BTN_PICK_REGION, m_hInstance, nullptr);
    m_regionControls.push_back(btnPick);
}

void SettingsDialog::createCounterTab(HWND hwnd) {
    int baseY = 45;
    int leftMargin = 25;
    
    // Nom du compteur
    HWND labelName = CreateWindowExW(0, L"STATIC", TR("counter_name"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 180, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_counterControls.push_back(labelName);
    
    HWND editName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        leftMargin + 185, baseY, 145, 24,
        hwnd, (HMENU)IDC_EDIT_COUNTER_NAME, m_hInstance, nullptr);
    m_counterControls.push_back(editName);
    
    baseY += 35;
    
    // Pas du compteur
    HWND labelStep = CreateWindowExW(0, L"STATIC", TR("counter_step"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 180, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_counterControls.push_back(labelStep);
    
    HWND editStep = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_RIGHT,
        leftMargin + 185, baseY, 80, 24,
        hwnd, (HMENU)IDC_EDIT_COUNTER_STEP, m_hInstance, nullptr);
    m_counterControls.push_back(editStep);
    
    HWND spinStep = CreateWindowExW(0, UPDOWN_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
        0, 0, 0, 0,
        hwnd, (HMENU)IDC_SPIN_STEP, m_hInstance, nullptr);
    SendMessage(spinStep, UDM_SETBUDDY, (WPARAM)editStep, 0);
    SendMessage(spinStep, UDM_SETRANGE32, 1, 1000);
    m_counterControls.push_back(spinStep);
    
    baseY += 40;
    
    // Son de notification
    HWND checkSound = CreateWindowExW(0, L"BUTTON", TR("sound_enabled"),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        leftMargin, baseY, 280, 24,
        hwnd, (HMENU)IDC_CHECK_SOUND_ENABLED, m_hInstance, nullptr);
    m_counterControls.push_back(checkSound);
    
    baseY += 32;
    
    HWND labelSound = CreateWindowExW(0, L"STATIC", TR("sound_type"),
        WS_CHILD | WS_VISIBLE,
        leftMargin + 20, baseY + 3, 40, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_counterControls.push_back(labelSound);
    
    HWND comboSound = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        leftMargin + 65, baseY, 180, 200,
        hwnd, (HMENU)IDC_COMBO_SOUND, m_hInstance, nullptr);
    SendMessageW(comboSound, CB_ADDSTRING, 0, (LPARAM)TR("sound_beep"));
    SendMessageW(comboSound, CB_ADDSTRING, 0, (LPARAM)TR("sound_ding"));
    SendMessageW(comboSound, CB_ADDSTRING, 0, (LPARAM)TR("sound_notification"));
    SendMessageW(comboSound, CB_SETCURSEL, 0, 0);
    m_counterControls.push_back(comboSound);
    
    HWND btnTest = CreateWindowExW(0, L"BUTTON", TR("test_sound"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin + 255, baseY - 2, 85, 26,
        hwnd, (HMENU)IDC_BTN_TEST_SOUND, m_hInstance, nullptr);
    m_counterControls.push_back(btnTest);
    
    baseY += 45;
    
    // Chemin de sauvegarde
    HWND labelSave = CreateWindowExW(0, L"STATIC", TR("save_path"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 180, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_counterControls.push_back(labelSave);
    
    baseY += 25;
    
    HWND editSavePath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
        leftMargin, baseY, 280, 24,
        hwnd, (HMENU)IDC_EDIT_SAVE_PATH, m_hInstance, nullptr);
    m_counterControls.push_back(editSavePath);
    
    HWND btnBrowse = CreateWindowExW(0, L"BUTTON", L"...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin + 290, baseY, 40, 24,
        hwnd, (HMENU)IDC_BTN_BROWSE_SAVE_PATH, m_hInstance, nullptr);
    m_counterControls.push_back(btnBrowse);
    
    baseY += 40;
    
    // Raccourci clavier global
    HWND labelHotkey = CreateWindowExW(0, L"STATIC", TR("hotkey_increment"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 180, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_counterControls.push_back(labelHotkey);
    
    baseY += 25;
    
    // Contrôle hotkey natif Windows
    HWND hotkeyCtrl = CreateWindowExW(0, HOTKEY_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        leftMargin, baseY, 200, 24,
        hwnd, (HMENU)IDC_HOTKEY_INCREMENT, m_hInstance, nullptr);
    m_counterControls.push_back(hotkeyCtrl);
    
    // Bouton pour effacer le raccourci
    HWND btnClear = CreateWindowExW(0, L"BUTTON", TR("clear"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        leftMargin + 210, baseY, 70, 24,
        hwnd, (HMENU)IDC_BTN_CLEAR_HOTKEY, m_hInstance, nullptr);
    m_counterControls.push_back(btnClear);
}

void SettingsDialog::createAdvancedTab(HWND hwnd) {
    int baseY = 45;
    int leftMargin = 25;
    
    // Méthode de correspondance
    HWND label1 = CreateWindowExW(0, L"STATIC", TR("match_method"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 170, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_advancedControls.push_back(label1);
    
    HWND comboMethod = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        leftMargin + 175, baseY, 200, 200,
        hwnd, (HMENU)IDC_COMBO_MATCH_METHOD, m_hInstance, nullptr);
    SendMessageW(comboMethod, CB_ADDSTRING, 0, (LPARAM)L"TM_CCOEFF_NORMED");
    SendMessageW(comboMethod, CB_ADDSTRING, 0, (LPARAM)L"TM_CCORR_NORMED");
    SendMessageW(comboMethod, CB_ADDSTRING, 0, (LPARAM)L"TM_SQDIFF_NORMED");
    SendMessageW(comboMethod, CB_SETCURSEL, 0, 0);
    m_advancedControls.push_back(comboMethod);
    
    baseY += 40;
    
    // Niveaux de gris
    HWND checkGray = CreateWindowExW(0, L"BUTTON", TR("use_grayscale"),
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        leftMargin, baseY, 320, 24,
        hwnd, (HMENU)IDC_CHECK_GRAYSCALE, m_hInstance, nullptr);
    m_advancedControls.push_back(checkGray);
    
    baseY += 40;
    
    // === Apparence ===
    HWND labelAppearance = CreateWindowExW(0, L"STATIC", TR("appearance_section"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY, 150, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_advancedControls.push_back(labelAppearance);
    
    baseY += 28;
    
    // Thème
    HWND labelTheme = CreateWindowExW(0, L"STATIC", TR("theme"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 80, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_advancedControls.push_back(labelTheme);
    
    HWND comboTheme = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        leftMargin + 85, baseY, 150, 200,
        hwnd, (HMENU)SettingsIDs::IDC_COMBO_THEME, m_hInstance, nullptr);
    SendMessageW(comboTheme, CB_ADDSTRING, 0, (LPARAM)TR("theme_system"));
    SendMessageW(comboTheme, CB_ADDSTRING, 0, (LPARAM)TR("theme_light"));
    SendMessageW(comboTheme, CB_ADDSTRING, 0, (LPARAM)TR("theme_dark"));
    SendMessageW(comboTheme, CB_SETCURSEL, (int)ThemeManager::getInstance().getThemeMode(), 0);
    m_advancedControls.push_back(comboTheme);
    
    baseY += 32;
    
    // Langue
    HWND labelLang = CreateWindowExW(0, L"STATIC", TR("language"),
        WS_CHILD | WS_VISIBLE,
        leftMargin, baseY + 3, 80, 20,
        hwnd, nullptr, m_hInstance, nullptr);
    m_advancedControls.push_back(labelLang);
    
    HWND comboLang = CreateWindowExW(0, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        leftMargin + 85, baseY, 150, 200,
        hwnd, (HMENU)SettingsIDs::IDC_COMBO_LANGUAGE, m_hInstance, nullptr);
    SendMessageW(comboLang, CB_ADDSTRING, 0, (LPARAM)L"Français");
    SendMessageW(comboLang, CB_ADDSTRING, 0, (LPARAM)L"English");
    SendMessageW(comboLang, CB_SETCURSEL, (WPARAM)(int)Localization::getInstance().getLanguage(), 0);
    m_advancedControls.push_back(comboLang);
}

void SettingsDialog::showTabPage(int tabIndex) {
    hideAllTabs();
    m_currentTab = tabIndex;
    
    // Ordre: 0=Compteur, 1=Détection, 2=Zone, 3=Avancé
    std::vector<HWND>* controls = nullptr;
    switch (tabIndex) {
        case 0: controls = &m_counterControls; break;
        case 1: controls = &m_detectionControls; break;
        case 2: controls = &m_regionControls; break;
        case 3: controls = &m_advancedControls; break;
    }
    
    if (controls) {
        for (HWND ctrl : *controls) {
            ShowWindow(ctrl, SW_SHOW);
        }
    }
}

void SettingsDialog::hideAllTabs() {
    for (HWND ctrl : m_detectionControls) ShowWindow(ctrl, SW_HIDE);
    for (HWND ctrl : m_regionControls) ShowWindow(ctrl, SW_HIDE);
    for (HWND ctrl : m_counterControls) ShowWindow(ctrl, SW_HIDE);
    for (HWND ctrl : m_advancedControls) ShowWindow(ctrl, SW_HIDE);
}

void SettingsDialog::onCommand(HWND hwnd, WPARAM wParam) {
    int wmId = LOWORD(wParam);
    
    switch (wmId) {
        case IDC_BTN_OK:
            if (validateSettings(hwnd)) {
                saveUIToConfig(hwnd);
                m_dialogResult = IDOK;
                DestroyWindow(hwnd);
            }
            break;
            
        case IDC_BTN_CANCEL:
            m_config = m_originalConfig;
            m_dialogResult = IDCANCEL;
            DestroyWindow(hwnd);
            break;
            
        case IDC_BTN_RESET_DEFAULTS:
            onResetDefaults(hwnd);
            break;
            
        case IDC_BTN_PICK_REGION:
            onPickRegion(hwnd);
            break;
            
        case IDC_BTN_TEST_SOUND:
            onTestSound(hwnd);
            break;
            
        case IDC_BTN_BROWSE_SAVE_PATH:
            onBrowseSavePath(hwnd);
            break;
            
        case IDC_BTN_CLEAR_HOTKEY:
            SendDlgItemMessageW(hwnd, IDC_HOTKEY_INCREMENT, HKM_SETHOTKEY, 0, 0);
            break;
            
        case IDC_RADIO_FULL_WINDOW:
            updateRegionControls(hwnd, false);
            break;
            
        case IDC_RADIO_CUSTOM_REGION:
            updateRegionControls(hwnd, true);
            break;
    }
}

void SettingsDialog::onNotify(HWND hwnd, LPARAM lParam) {
    auto* nmhdr = reinterpret_cast<NMHDR*>(lParam);
    
    if (nmhdr->idFrom == IDC_TAB_CONTROL && nmhdr->code == TCN_SELCHANGE) {
        int sel = TabCtrl_GetCurSel(m_tabControl);
        showTabPage(sel);
    }
}

void SettingsDialog::onHScroll(HWND hwnd, WPARAM wParam, LPARAM lParam) {
    HWND slider = (HWND)lParam;
    if (GetDlgCtrlID(slider) == IDC_SLIDER_THRESHOLD) {
        updateThresholdDisplay(hwnd);
    }
}

void SettingsDialog::updateThresholdDisplay(HWND hwnd) {
    HWND slider = GetDlgItem(hwnd, IDC_SLIDER_THRESHOLD);
    HWND label = GetDlgItem(hwnd, IDC_STATIC_THRESHOLD_VALUE);
    
    int value = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
    std::wstring text = std::to_wstring(value) + L"%";
    SetWindowTextW(label, text.c_str());
}

void SettingsDialog::updateRegionControls(HWND hwnd, bool enabled) {
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_REGION_X), enabled);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_REGION_Y), enabled);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_REGION_W), enabled);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_REGION_H), enabled);
    EnableWindow(GetDlgItem(hwnd, IDC_BTN_PICK_REGION), enabled);
}

void SettingsDialog::loadConfigToUI(HWND hwnd) {
    // Onglet Détection
    SendDlgItemMessage(hwnd, IDC_SLIDER_THRESHOLD, TBM_SETPOS, TRUE, 
        (int)(m_config.detection.threshold * 100));
    updateThresholdDisplay(hwnd);
    
    // Convertir ms en secondes pour l'affichage
    wchar_t scanBuf[32], coolBuf[32];
    swprintf_s(scanBuf, L"%.2f", m_config.detection.scanIntervalMs / 1000.0);
    swprintf_s(coolBuf, L"%.1f", m_config.detection.cooldownMs / 1000.0);
    SetDlgItemTextW(hwnd, IDC_EDIT_SCAN_INTERVAL, scanBuf);
    SetDlgItemTextW(hwnd, IDC_EDIT_COOLDOWN, coolBuf);
    
    CheckDlgButton(hwnd, IDC_CHECK_DETECT_MULTIPLE, 
        m_config.detection.detectMultiple ? BST_CHECKED : BST_UNCHECKED);
    
    // Onglet Zone
    CheckDlgButton(hwnd, IDC_RADIO_FULL_WINDOW, 
        m_config.region.useFullWindow ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwnd, IDC_RADIO_CUSTOM_REGION, 
        !m_config.region.useFullWindow ? BST_CHECKED : BST_UNCHECKED);
    
    SetDlgItemInt(hwnd, IDC_EDIT_REGION_X, m_config.region.x, FALSE);
    SetDlgItemInt(hwnd, IDC_EDIT_REGION_Y, m_config.region.y, FALSE);
    SetDlgItemInt(hwnd, IDC_EDIT_REGION_W, m_config.region.width, FALSE);
    SetDlgItemInt(hwnd, IDC_EDIT_REGION_H, m_config.region.height, FALSE);
    
    updateRegionControls(hwnd, !m_config.region.useFullWindow);
    
    // Onglet Compteur
    SetDlgItemTextW(hwnd, IDC_EDIT_COUNTER_NAME, m_config.counterName.c_str());
    CheckDlgButton(hwnd, IDC_CHECK_SOUND_ENABLED, 
        m_config.soundEnabled ? BST_CHECKED : BST_UNCHECKED);
    SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_SETCURSEL, m_config.soundIndex, 0);
    SetDlgItemInt(hwnd, IDC_EDIT_COUNTER_STEP, m_config.detection.counterStep, FALSE);
    SetDlgItemTextW(hwnd, IDC_EDIT_SAVE_PATH, m_config.savePath.c_str());
    
    // Raccourci clavier
    if (m_config.hotkeyVirtualKey != 0) {
        WORD hk = MAKEWORD(m_config.hotkeyVirtualKey, 
            ((m_config.hotkeyModifiers & MOD_SHIFT) ? HOTKEYF_SHIFT : 0) |
            ((m_config.hotkeyModifiers & MOD_CONTROL) ? HOTKEYF_CONTROL : 0) |
            ((m_config.hotkeyModifiers & MOD_ALT) ? HOTKEYF_ALT : 0));
        SendDlgItemMessageW(hwnd, IDC_HOTKEY_INCREMENT, HKM_SETHOTKEY, hk, 0);
    }
    
    // Onglet Avancé
    SendDlgItemMessage(hwnd, IDC_COMBO_MATCH_METHOD, CB_SETCURSEL, m_config.matchMethod, 0);
    CheckDlgButton(hwnd, IDC_CHECK_GRAYSCALE, 
        m_config.useGrayscale ? BST_CHECKED : BST_UNCHECKED);
}

void SettingsDialog::saveUIToConfig(HWND hwnd) {
    // Onglet Détection
    int threshold = (int)SendDlgItemMessage(hwnd, IDC_SLIDER_THRESHOLD, TBM_GETPOS, 0, 0);
    m_config.detection.threshold = threshold / 100.0;
    
    // Convertir les secondes en millisecondes
    wchar_t scanBuf[32], coolBuf[32];
    GetDlgItemTextW(hwnd, IDC_EDIT_SCAN_INTERVAL, scanBuf, 32);
    GetDlgItemTextW(hwnd, IDC_EDIT_COOLDOWN, coolBuf, 32);
    m_config.detection.scanIntervalMs = (int)(_wtof(scanBuf) * 1000);
    m_config.detection.cooldownMs = (int)(_wtof(coolBuf) * 1000);
    
    m_config.detection.detectMultiple = (IsDlgButtonChecked(hwnd, IDC_CHECK_DETECT_MULTIPLE) == BST_CHECKED);
    
    // Onglet Zone
    m_config.region.useFullWindow = (IsDlgButtonChecked(hwnd, IDC_RADIO_FULL_WINDOW) == BST_CHECKED);
    m_config.region.x = GetDlgItemInt(hwnd, IDC_EDIT_REGION_X, nullptr, FALSE);
    m_config.region.y = GetDlgItemInt(hwnd, IDC_EDIT_REGION_Y, nullptr, FALSE);
    m_config.region.width = GetDlgItemInt(hwnd, IDC_EDIT_REGION_W, nullptr, FALSE);
    m_config.region.height = GetDlgItemInt(hwnd, IDC_EDIT_REGION_H, nullptr, FALSE);
    
    // Onglet Compteur
    wchar_t counterName[256] = {};
    GetDlgItemTextW(hwnd, IDC_EDIT_COUNTER_NAME, counterName, 256);
    m_config.counterName = counterName;
    
    m_config.soundEnabled = (IsDlgButtonChecked(hwnd, IDC_CHECK_SOUND_ENABLED) == BST_CHECKED);
    m_config.soundIndex = (int)SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_GETCURSEL, 0, 0);
    m_config.detection.counterStep = GetDlgItemInt(hwnd, IDC_EDIT_COUNTER_STEP, nullptr, FALSE);
    if (m_config.detection.counterStep < 1) m_config.detection.counterStep = 1;
    
    // Chemin de sauvegarde
    wchar_t savePath[MAX_PATH] = {};
    GetDlgItemTextW(hwnd, IDC_EDIT_SAVE_PATH, savePath, MAX_PATH);
    m_config.savePath = savePath;
    
    // Raccourci clavier
    WORD hk = (WORD)SendDlgItemMessageW(hwnd, IDC_HOTKEY_INCREMENT, HKM_GETHOTKEY, 0, 0);
    BYTE vk = LOBYTE(hk);
    BYTE mod = HIBYTE(hk);
    m_config.hotkeyVirtualKey = vk;
    m_config.hotkeyModifiers = 0;
    if (mod & HOTKEYF_SHIFT) m_config.hotkeyModifiers |= MOD_SHIFT;
    if (mod & HOTKEYF_CONTROL) m_config.hotkeyModifiers |= MOD_CONTROL;
    if (mod & HOTKEYF_ALT) m_config.hotkeyModifiers |= MOD_ALT;
    
    // Onglet Avancé
    m_config.matchMethod = (int)SendDlgItemMessage(hwnd, IDC_COMBO_MATCH_METHOD, CB_GETCURSEL, 0, 0);
    m_config.useGrayscale = (IsDlgButtonChecked(hwnd, IDC_CHECK_GRAYSCALE) == BST_CHECKED);
    
    // Thème et langue
    int themeIndex = (int)SendDlgItemMessage(hwnd, SettingsIDs::IDC_COMBO_THEME, CB_GETCURSEL, 0, 0);
    ThemeManager::getInstance().setThemeMode((ThemeMode)themeIndex);
    
    int langIndex = (int)SendDlgItemMessage(hwnd, SettingsIDs::IDC_COMBO_LANGUAGE, CB_GETCURSEL, 0, 0);
    Localization::getInstance().setLanguage((Language)langIndex);
}

bool SettingsDialog::validateSettings(HWND hwnd) {
    // Valider l'intervalle de scan (0.05s - 5s)
    wchar_t scanBuf[32];
    GetDlgItemTextW(hwnd, IDC_EDIT_SCAN_INTERVAL, scanBuf, 32);
    double scanInterval = _wtof(scanBuf);
    if (scanInterval < 0.05 || scanInterval > 5.0) {
        MessageBoxW(hwnd, TR("error_scan_interval"), 
            TR("validation"), MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_SCAN_INTERVAL));
        return false;
    }
    
    // Valider le cooldown (0s - 300s)
    wchar_t coolBuf[32];
    GetDlgItemTextW(hwnd, IDC_EDIT_COOLDOWN, coolBuf, 32);
    double cooldown = _wtof(coolBuf);
    if (cooldown < 0 || cooldown > 300.0) {
        MessageBoxW(hwnd, TR("error_cooldown"), 
            TR("validation"), MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_COOLDOWN));
        return false;
    }
    
    return true;
}

void SettingsDialog::onPickRegion(HWND hwnd) {
    if (!m_targetWindow || !IsWindow(m_targetWindow)) {
        MessageBoxW(hwnd, TR("error_no_window"),
            TR("warning"), MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Masquer temporairement la boîte de dialogue
    ShowWindow(hwnd, SW_HIDE);
    Sleep(200);
    
    RegionSelector selector(m_targetWindow);
    if (selector.select()) {
        CaptureRegion region = selector.getRegion();
        SetDlgItemInt(hwnd, IDC_EDIT_REGION_X, region.x, FALSE);
        SetDlgItemInt(hwnd, IDC_EDIT_REGION_Y, region.y, FALSE);
        SetDlgItemInt(hwnd, IDC_EDIT_REGION_W, region.width, FALSE);
        SetDlgItemInt(hwnd, IDC_EDIT_REGION_H, region.height, FALSE);
        
        CheckDlgButton(hwnd, IDC_RADIO_FULL_WINDOW, BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_RADIO_CUSTOM_REGION, BST_CHECKED);
        updateRegionControls(hwnd, true);
    }
    
    ShowWindow(hwnd, SW_SHOW);
}

void SettingsDialog::onTestSound(HWND hwnd) {
    int soundIndex = (int)SendDlgItemMessage(hwnd, IDC_COMBO_SOUND, CB_GETCURSEL, 0, 0);
    
    switch (soundIndex) {
        case 0:
            MessageBeep(MB_OK);
            break;
        case 1:
            MessageBeep(MB_ICONASTERISK);
            break;
        case 2:
            PlaySoundW(L"SystemNotification", nullptr, SND_ALIAS | SND_ASYNC);
            break;
    }
}

void SettingsDialog::onResetDefaults(HWND hwnd) {
    m_config = ExtendedConfig();
    loadConfigToUI(hwnd);
    MessageBoxW(hwnd, TR("status_defaults_reset"), 
        TR("default"), MB_OK | MB_ICONINFORMATION);
}

void SettingsDialog::onBrowseSavePath(HWND hwnd) {
    // Ouvrir le dialogue de sélection de fichier
    wchar_t szFile[MAX_PATH] = {};
    
    // Pré-remplir avec le chemin actuel
    GetDlgItemTextW(hwnd, IDC_EDIT_SAVE_PATH, szFile, MAX_PATH);
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Fichiers texte (*.txt)\0*.txt\0Tous les fichiers (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    
    if (GetSaveFileNameW(&ofn)) {
        SetDlgItemTextW(hwnd, IDC_EDIT_SAVE_PATH, szFile);
        m_config.savePath = szFile;
    }
}

// ============================================================================
// RegionSelector Implementation
// ============================================================================

RegionSelector::RegionSelector(HWND targetWindow) 
    : m_targetWindow(targetWindow) {}

RegionSelector::~RegionSelector() {
    if (m_overlayWindow && IsWindow(m_overlayWindow)) {
        DestroyWindow(m_overlayWindow);
    }
}

bool RegionSelector::select() {
    // Obtenir les dimensions de la zone client de la fenêtre cible
    RECT clientRect;
    GetClientRect(m_targetWindow, &clientRect);
    
    // Convertir en coordonnées écran
    POINT topLeft = {clientRect.left, clientRect.top};
    POINT bottomRight = {clientRect.right, clientRect.bottom};
    ClientToScreen(m_targetWindow, &topLeft);
    ClientToScreen(m_targetWindow, &bottomRight);
    
    int overlayX = topLeft.x;
    int overlayY = topLeft.y;
    int overlayWidth = bottomRight.x - topLeft.x;
    int overlayHeight = bottomRight.y - topLeft.y;
    
    // Créer une fenêtre overlay transparente
    const wchar_t* className = L"RegionSelectorOverlay";
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = className;
    
    RegisterClassExW(&wc);
    
    m_overlayWindow = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        className,
        L"",
        WS_POPUP,
        overlayX, overlayY,
        overlayWidth, overlayHeight,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );
    
    if (!m_overlayWindow) {
        return false;
    }
    
    // Rendre la fenêtre semi-transparente
    SetLayeredWindowAttributes(m_overlayWindow, 0, 100, LWA_ALPHA);
    
    ShowWindow(m_overlayWindow, SW_SHOW);
    UpdateWindow(m_overlayWindow);
    SetCapture(m_overlayWindow);
    
    // Boucle de messages
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) && !m_completed && !m_cancelled) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    ReleaseCapture();
    DestroyWindow(m_overlayWindow);
    m_overlayWindow = nullptr;
    
    return m_completed && !m_cancelled;
}

LRESULT CALLBACK RegionSelector::OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    RegionSelector* pThis = nullptr;
    
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<RegionSelector*>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<RegionSelector*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (pThis) {
        switch (msg) {
            case WM_PAINT:
                pThis->onPaint(hwnd);
                return 0;
                
            case WM_LBUTTONDOWN:
                pThis->onMouseDown(hwnd, LOWORD(lParam), HIWORD(lParam));
                return 0;
                
            case WM_MOUSEMOVE:
                pThis->onMouseMove(hwnd, LOWORD(lParam), HIWORD(lParam));
                return 0;
                
            case WM_LBUTTONUP:
                pThis->onMouseUp(hwnd, LOWORD(lParam), HIWORD(lParam));
                return 0;
                
            case WM_RBUTTONDOWN:
            case WM_KEYDOWN:
                if (msg == WM_RBUTTONDOWN || wParam == VK_ESCAPE) {
                    pThis->m_cancelled = true;
                    PostQuitMessage(0);
                }
                return 0;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void RegionSelector::onPaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Fond semi-transparent
    HBRUSH bgBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Dessiner la sélection si en cours
    if (m_selecting) {
        int left = (std::min)(m_startPoint.x, m_currentPoint.x);
        int top = (std::min)(m_startPoint.y, m_currentPoint.y);
        int right = (std::max)(m_startPoint.x, m_currentPoint.x);
        int bottom = (std::max)(m_startPoint.y, m_currentPoint.y);
        
        RECT selRect = {left, top, right, bottom};
        
        // Remplir la sélection avec une couleur claire
        HBRUSH selBrush = CreateSolidBrush(RGB(0, 120, 215));
        FrameRect(hdc, &selRect, selBrush);
        DeleteObject(selBrush);
        
        // Afficher les dimensions
        std::wstringstream ss;
        ss << (right - left) << L" x " << (bottom - top);
        
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, left + 5, top + 5, ss.str().c_str(), (int)ss.str().length());
    }
    
    // Instructions
    const wchar_t* instructions = L"Dessinez un rectangle pour sélectionner la zone. Échap ou clic droit pour annuler.";
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, 10, 10, instructions, (int)wcslen(instructions));
    
    EndPaint(hwnd, &ps);
}

void RegionSelector::onMouseDown(HWND hwnd, int x, int y) {
    m_selecting = true;
    m_startPoint = {x, y};
    m_currentPoint = {x, y};
    InvalidateRect(hwnd, nullptr, TRUE);
}

void RegionSelector::onMouseMove(HWND hwnd, int x, int y) {
    if (m_selecting) {
        m_currentPoint = {x, y};
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

void RegionSelector::onMouseUp(HWND hwnd, int x, int y) {
    if (m_selecting) {
        m_selecting = false;
        m_currentPoint = {x, y};
        
        int left = (std::min)(m_startPoint.x, m_currentPoint.x);
        int top = (std::min)(m_startPoint.y, m_currentPoint.y);
        int right = (std::max)(m_startPoint.x, m_currentPoint.x);
        int bottom = (std::max)(m_startPoint.y, m_currentPoint.y);
        
        int width = right - left;
        int height = bottom - top;
        
        // Vérifier que la sélection est valide
        if (width > 10 && height > 10) {
            m_region.x = left;
            m_region.y = top;
            m_region.width = width;
            m_region.height = height;
            m_region.useFullWindow = false;
            m_completed = true;
        }
        
        PostQuitMessage(0);
    }
}
