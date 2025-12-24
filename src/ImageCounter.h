#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>
#include <memory>
#include <map>

#include <opencv2/opencv.hpp>

// ============================================================================
// Langues disponibles
// ============================================================================
enum class Language {
    French,
    English
};

// Classe de localisation (singleton)
class Localization {
public:
    static Localization& getInstance() {
        static Localization instance;
        return instance;
    }
    
    void setLanguage(Language lang) {
        m_currentLanguage = lang;
    }
    
    Language getLanguage() const {
        return m_currentLanguage;
    }
    
    const wchar_t* get(const std::wstring& key) const {
        const auto& strings = (m_currentLanguage == Language::French) ? m_french : m_english;
        auto it = strings.find(key);
        if (it != strings.end()) {
            return it->second.c_str();
        }
        return key.c_str();
    }

private:
    Localization();
    Language m_currentLanguage = Language::French;
    std::map<std::wstring, std::wstring> m_french;
    std::map<std::wstring, std::wstring> m_english;
};

#define TR(key) Localization::getInstance().get(L##key)

// ============================================================================
// Types de thème
// ============================================================================
enum class ThemeMode {
    System,
    Light,
    Dark
};

// Préférence de dark mode pour l'application (API non documentée Windows 10 1903+)
enum PreferredAppMode {
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND hWnd, bool allow);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();

// Classe de gestion du thème
class ThemeManager {
public:
    static ThemeManager& getInstance() {
        static ThemeManager instance;
        return instance;
    }
    
    void setThemeMode(ThemeMode mode) {
        m_themeMode = mode;
        m_isDark = shouldUseDarkTheme();
        
        // Appliquer le mode au niveau de l'application
        if (m_setPreferredAppMode) {
            if (m_isDark) {
                m_setPreferredAppMode(ForceDark);
            } else {
                m_setPreferredAppMode(ForceLight);
            }
        }
        if (m_refreshImmersiveColorPolicyState) {
            m_refreshImmersiveColorPolicyState();
        }
    }
    
    ThemeMode getThemeMode() const {
        return m_themeMode;
    }
    
    bool isDarkTheme() const {
        return m_isDark;
    }
    
    void applyToWindow(HWND hwnd) {
        m_isDark = shouldUseDarkTheme();
        BOOL useDark = m_isDark ? TRUE : FALSE;
        
        // Appliquer à la barre de titre
        DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
        
        // Permettre le dark mode pour cette fenêtre
        if (m_allowDarkModeForWindow) {
            m_allowDarkModeForWindow(hwnd, m_isDark);
        }
        
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    
    void allowDarkModeForControl(HWND hwnd) {
        if (m_allowDarkModeForWindow && m_isDark) {
            m_allowDarkModeForWindow(hwnd, true);
            SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
        }
    }
    
    static bool isSystemDarkTheme() {
        HKEY hKey;
        DWORD value = 1;
        DWORD size = sizeof(value);
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&value, &size);
            RegCloseKey(hKey);
        }
        return value == 0;
    }

private:
    ThemeManager() : m_themeMode(ThemeMode::System), m_isDark(false) {
        // Charger les fonctions dark mode de uxtheme.dll
        HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (hUxtheme) {
            m_setPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
            m_allowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));
            m_refreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));
        }
        
        m_isDark = shouldUseDarkTheme();
        
        // Initialiser le mode dark au démarrage
        if (m_setPreferredAppMode) {
            m_setPreferredAppMode(AllowDark);
        }
    }
    
    bool shouldUseDarkTheme() const {
        switch (m_themeMode) {
            case ThemeMode::Light: return false;
            case ThemeMode::Dark: return true;
            default: return isSystemDarkTheme();
        }
    }
    
    ThemeMode m_themeMode;
    bool m_isDark;
    fnSetPreferredAppMode m_setPreferredAppMode = nullptr;
    fnAllowDarkModeForWindow m_allowDarkModeForWindow = nullptr;
    fnRefreshImmersiveColorPolicyState m_refreshImmersiveColorPolicyState = nullptr;
};

// ============================================================================
// Configuration de détection
// ============================================================================
struct DetectionConfig {
    double threshold = 0.85;
    int scanIntervalMs = 500;
    int cooldownMs = 1000;
    bool detectMultiple = false;
    int counterStep = 1;
    bool useGrayscale = false;
    int matchMethod = 0;  // 0 = TM_CCOEFF_NORMED, 1 = TM_CCORR_NORMED, 2 = TM_SQDIFF_NORMED
};

// Zone de capture
struct CaptureRegion {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool useFullWindow = true;
};

// Résultat de détection
struct DetectionResult {
    bool found = false;
    double confidence = 0.0;
    POINT location = {0, 0};
    ULONGLONG timestamp = 0;
};

// Classe principale de détection d'image
class ImageDetector {
public:
    ImageDetector();
    ~ImageDetector();

    // Gestion de l'image de référence
    bool loadReferenceImage(const std::wstring& path);
    bool setReferenceFromClipboard();
    void setReferenceFromMat(const cv::Mat& image);
    void clearReferenceImage();
    bool hasReferenceImage() const;
    cv::Mat getReferenceImage() const;

    // Gestion de la fenêtre cible
    bool setTargetWindow(HWND hwnd);
    bool setTargetByTitle(const std::wstring& title);
    HWND getTargetWindow() const;
    static std::vector<std::pair<HWND, std::wstring>> enumerateWindows();

    // Configuration
    void setConfig(const DetectionConfig& config);
    DetectionConfig getConfig() const;
    void setCaptureRegion(const CaptureRegion& region);
    CaptureRegion getCaptureRegion() const;

    // Capture et détection
    cv::Mat captureWindow();
    cv::Mat captureRegion();
    DetectionResult detectImage(const cv::Mat& source);

    // Contrôle du scan automatique
    void startScanning();
    void stopScanning();
    bool isScanning() const;

    // Compteur
    int getCounter() const;
    void resetCounter();
    void setCounter(int value);
    
    // Dernier score de confiance
    double getLastConfidence() const { return m_lastConfidence; }

    // Callback pour notification de détection
    using DetectionCallback = std::function<void(const DetectionResult&, int counter)>;
    void setDetectionCallback(DetectionCallback callback);

private:
    void scanThread();
    cv::Mat hwndToMat(HWND hwnd);
    cv::Mat hwndToMat(HWND hwnd, const CaptureRegion& region);

    cv::Mat m_referenceImage;
    HWND m_targetWindow = nullptr;
    DetectionConfig m_config;
    CaptureRegion m_captureRegion;

    std::atomic<bool> m_scanning{false};
    std::atomic<int> m_counter{0};
    std::atomic<double> m_lastConfidence{0.0};
    std::thread m_scanThread;
    mutable std::mutex m_mutex;

    ULONGLONG m_lastDetectionTime = 0;
    DetectionCallback m_callback;
};

// Callback étendu qui reçoit aussi le score même sans détection
using ScanCallback = std::function<void(const DetectionResult&, int counter, bool detected)>;

// Structure pour un onglet/compteur
// Entrée d'historique de détection
struct DetectionEntry {
    SYSTEMTIME timestamp;
    int counterValue;
    double confidence;
    std::wstring windowTitle;
};

struct CounterTab {
    std::wstring name = L"Compteur";
    std::unique_ptr<ImageDetector> detector;
    HWND targetWindow = nullptr;
    int selectedWindowIndex = -1;
    HBITMAP previewBitmap = nullptr;
    HBITMAP captureBitmap = nullptr;
    
    // Configuration spécifique à l'onglet
    DetectionConfig config;
    CaptureRegion region;
    
    // Son
    bool soundEnabled = false;
    int soundIndex = 0;
    
    // Fichier de sauvegarde
    std::wstring saveFilePath;
    
    // Image de référence
    std::wstring referenceImagePath;
    
    // Fenêtre cible (nom pour restauration)
    std::wstring targetWindowTitle;
    
    // Raccourci clavier global pour incrémenter
    UINT hotkeyVirtualKey = 0;      // 0 = pas de raccourci
    UINT hotkeyModifiers = 0;       // MOD_CONTROL, MOD_SHIFT, MOD_ALT
    
    // État
    bool isRunning = false;
    ULONGLONG lastDetectionTime = 0;  // Temps de la dernière détection
    
    // Historique des détections
    std::vector<DetectionEntry> detectionHistory;
    
    CounterTab() : detector(std::make_unique<ImageDetector>()) {}
    ~CounterTab() {
        if (previewBitmap) {
            DeleteObject(previewBitmap);
        }
        if (captureBitmap) {
            DeleteObject(captureBitmap);
        }
    }
    
    // Pas de copie, uniquement déplacement
    CounterTab(const CounterTab&) = delete;
    CounterTab& operator=(const CounterTab&) = delete;
    CounterTab(CounterTab&& other) noexcept = default;
    CounterTab& operator=(CounterTab&& other) noexcept = default;
};

// Classe de gestion de l'interface Win32
class MainWindow {
public:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();

    bool create();
    int run();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void onCreate(HWND hwnd);
    void onCommand(HWND hwnd, WPARAM wParam);
    void onNotify(HWND hwnd, LPARAM lParam);
    void onPaint(HWND hwnd);
    void onTimer(HWND hwnd);
    void onHotkey(WPARAM wParam);
    void onDetection(int tabIndex, const DetectionResult& result, int counter);
    
    // Hotkey global
    void registerHotkeys();
    void unregisterHotkeys();
    void incrementCounter();
    void incrementCounterForTab(int tabIndex);

    // Gestion des onglets
    void addNewTab();
    void removeCurrentTab();
    void switchToTab(int index);
    void updateTabControl();
    
    // Mise à jour de l'UI pour l'onglet courant
    void updateWindowList();
    void updateUIForCurrentTab();
    void syncUIToTab();
    void syncTabFromUI();
    
    // Actions
    void loadReferenceImage();
    void quickCapture();
    void selectCaptureRegion();
    void showSettings(int initialTab = 0);
    void startScanning();
    void stopScanning();
    void resetCounter();
    void editCounter();
    
    // Historique des détections
    void showHistory();
    void exportHistoryToCSV(CounterTab* tab);
    void clearHistory(CounterTab* tab);
    void addDetectionToHistory(CounterTab* tab, int counterValue, double confidence);
    
    // Langue et thème
    void refreshUILanguage();
    void applyTheme();
    
    // Sauvegarde du compteur
    void saveCounterToFile(CounterTab* tab);
    void loadCounterFromFile(CounterTab* tab);
    std::wstring getDefaultSavePath(const std::wstring& tabName);
    
    // Sauvegarde/restauration des paramètres
    void saveAllSettings();
    void loadAllSettings();
    std::wstring getSettingsFilePath();
    void saveTabSettings(CounterTab* tab, const std::wstring& basePath);
    void loadTabSettings(CounterTab* tab, const std::wstring& basePath);
    
    // Utilitaires
    CounterTab* currentTab();
    void updatePreviewForTab(CounterTab* tab, const cv::Mat& image);
    void updateCapturePreview(CounterTab* tab);

    HINSTANCE m_hInstance;
    HWND m_hwnd = nullptr;
    
    // Contrôle d'onglets
    HWND m_tabControl = nullptr;
    HWND m_btnAddTab = nullptr;
    HWND m_btnRemoveTab = nullptr;
    
    // Contrôles de contenu (partagés, mis à jour selon l'onglet actif)
    HWND m_comboWindows = nullptr;
    HWND m_btnRefreshWindows = nullptr;
    HWND m_btnLoadImage = nullptr;
    HWND m_btnQuickCapture = nullptr;
    HWND m_btnSelectRegion = nullptr;
    HWND m_btnStart = nullptr;
    HWND m_btnStop = nullptr;
    HWND m_btnReset = nullptr;
    HWND m_btnSettings = nullptr;
    HWND m_btnHistory = nullptr;
    HWND m_staticCounter = nullptr;
    HWND m_staticStep = nullptr;
    HWND m_btnCounterMinus = nullptr;
    HWND m_btnCounterPlus = nullptr;
    HWND m_staticStatus = nullptr;
    HWND m_staticPreview = nullptr;
    HWND m_staticCapture = nullptr;
    
    // Labels traduisibles
    HWND m_labelTargetWindow = nullptr;
    HWND m_labelCounter = nullptr;
    HWND m_labelRefImage = nullptr;
    HWND m_labelCapture = nullptr;
    
    // Données
    std::vector<std::unique_ptr<CounterTab>> m_tabs;
    int m_currentTabIndex = 0;
    std::vector<std::pair<HWND, std::wstring>> m_windowList;
    
    // Police pour le compteur
    HFONT m_counterFont = nullptr;
    HFONT m_buttonFont = nullptr;
    
    // Thème
    HBRUSH m_backgroundBrush = nullptr;

    // IDs des contrôles
    static constexpr int IDC_TAB_CONTROL = 100;
    static constexpr int IDC_BTN_ADD_TAB = 101;
    static constexpr int IDC_BTN_REMOVE_TAB = 102;
    static constexpr int IDC_COMBO_WINDOWS = 104;
    static constexpr int IDC_BTN_REFRESH_WINDOWS = 105;
    static constexpr int IDC_BTN_LOAD_IMAGE = 106;
    static constexpr int IDC_BTN_QUICK_CAPTURE = 107;
    static constexpr int IDC_BTN_SELECT_REGION = 108;
    static constexpr int IDC_BTN_START = 109;
    static constexpr int IDC_BTN_STOP = 110;
    static constexpr int IDC_BTN_RESET = 111;
    static constexpr int IDC_BTN_SETTINGS = 112;
    static constexpr int IDC_STATIC_COUNTER = 113;
    static constexpr int IDC_STATIC_STATUS = 114;
    static constexpr int IDC_STATIC_PREVIEW = 115;
    static constexpr int IDC_BTN_EDIT_COUNTER = 116;
    static constexpr int IDC_STATIC_STEP = 117;
    static constexpr int IDC_STATIC_CAPTURE = 118;
    static constexpr int IDC_BTN_COUNTER_MINUS = 119;
    static constexpr int IDC_BTN_COUNTER_PLUS = 120;
    static constexpr int IDC_BTN_HISTORY = 121;
    static constexpr int IDT_UPDATE = 1;
    
    // Hotkey global
    static constexpr int HOTKEY_INCREMENT = 1;
};

// Dialogue de renommage d'onglet
class RenameDialog {
public:
    static bool show(HWND parent, std::wstring& name);
private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static std::wstring* s_namePtr;
    static HBRUSH s_darkBrush;
};
