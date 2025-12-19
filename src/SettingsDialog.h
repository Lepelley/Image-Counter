#pragma once

#include "ImageCounter.h"

// IDs des contrôles de la boîte de dialogue
namespace SettingsIDs {
    constexpr int IDD_SETTINGS = 200;
    
    // Onglet Détection
    constexpr int IDC_SLIDER_THRESHOLD = 201;
    constexpr int IDC_STATIC_THRESHOLD_VALUE = 202;
    constexpr int IDC_EDIT_SCAN_INTERVAL = 203;
    constexpr int IDC_SPIN_SCAN_INTERVAL = 204;
    constexpr int IDC_EDIT_COOLDOWN = 205;
    constexpr int IDC_SPIN_COOLDOWN = 206;
    constexpr int IDC_CHECK_DETECT_MULTIPLE = 207;
    
    // Onglet Zone de capture
    constexpr int IDC_RADIO_FULL_WINDOW = 210;
    constexpr int IDC_RADIO_CUSTOM_REGION = 211;
    constexpr int IDC_EDIT_REGION_X = 212;
    constexpr int IDC_EDIT_REGION_Y = 213;
    constexpr int IDC_EDIT_REGION_W = 214;
    constexpr int IDC_EDIT_REGION_H = 215;
    constexpr int IDC_BTN_PICK_REGION = 216;
    constexpr int IDC_STATIC_REGION_PREVIEW = 217;
    
    // Onglet Compteur
    constexpr int IDC_EDIT_COUNTER_NAME = 238;
    constexpr int IDC_EDIT_COUNTER_VALUE = 220;
    constexpr int IDC_SPIN_COUNTER = 221;
    constexpr int IDC_CHECK_SOUND_ENABLED = 222;
    constexpr int IDC_COMBO_SOUND = 223;
    constexpr int IDC_BTN_TEST_SOUND = 224;
    constexpr int IDC_CHECK_AUTO_SAVE = 225;
    constexpr int IDC_EDIT_COUNTER_STEP = 226;
    constexpr int IDC_SPIN_STEP = 227;
    constexpr int IDC_BTN_BROWSE_SAVE_PATH = 228;
    constexpr int IDC_EDIT_SAVE_PATH = 229;
    constexpr int IDC_HOTKEY_INCREMENT = 236;
    constexpr int IDC_BTN_CLEAR_HOTKEY = 237;
    
    // Onglet Avancé
    constexpr int IDC_COMBO_MATCH_METHOD = 230;
    constexpr int IDC_CHECK_GRAYSCALE = 231;
    constexpr int IDC_CHECK_DEBUG_MODE = 232;
    constexpr int IDC_BTN_EXPORT_LOG = 233;
    constexpr int IDC_COMBO_THEME = 234;
    constexpr int IDC_COMBO_LANGUAGE = 235;
    
    // Boutons communs
    constexpr int IDC_BTN_OK = 250;
    constexpr int IDC_BTN_CANCEL = 251;
    constexpr int IDC_BTN_APPLY = 252;
    constexpr int IDC_BTN_RESET_DEFAULTS = 253;
    
    // Onglets
    constexpr int IDC_TAB_CONTROL = 260;
}

// Configuration étendue
struct ExtendedConfig {
    DetectionConfig detection;
    CaptureRegion region;
    
    // Nom du compteur
    std::wstring counterName;
    
    // Son
    bool soundEnabled = false;
    int soundIndex = 0;  // 0 = Beep système, 1+ = sons personnalisés
    
    // Avancé
    int matchMethod = 0;  // 0 = TM_CCOEFF_NORMED, 1 = TM_CCORR_NORMED, etc.
    bool useGrayscale = false;
    
    // Sauvegarde
    std::wstring savePath;
    
    // Raccourci clavier
    UINT hotkeyVirtualKey = 0;
    UINT hotkeyModifiers = 0;
};

// Classe pour la boîte de dialogue des paramètres
class SettingsDialog {
public:
    SettingsDialog(HWND parent, ExtendedConfig& config, HWND targetWindow = nullptr);
    ~SettingsDialog();
    
    // Afficher la boîte de dialogue (modal)
    // Retourne true si l'utilisateur a cliqué OK, false sinon
    bool show();
    
    // Définir l'onglet initial (0=Détection, 1=Zone, 2=Compteur, 3=Avancé)
    void setInitialTab(int tab) { m_initialTab = tab; }
    
    // Récupérer la configuration modifiée
    ExtendedConfig getConfig() const { return m_config; }

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Handlers de messages
    void onInitDialog(HWND hwnd);
    void onCommand(HWND hwnd, WPARAM wParam);
    void onNotify(HWND hwnd, LPARAM lParam);
    void onHScroll(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void onTabSelChange(HWND hwnd);
    
    // Création des contrôles
    void createTabControl(HWND hwnd);
    void createDetectionTab(HWND hwnd);
    void createRegionTab(HWND hwnd);
    void createCounterTab(HWND hwnd);
    void createAdvancedTab(HWND hwnd);
    
    // Gestion des onglets
    void showTabPage(int tabIndex);
    void hideAllTabs();
    
    // Mise à jour de l'UI
    void updateThresholdDisplay(HWND hwnd);
    void updateRegionControls(HWND hwnd, bool enabled);
    void loadConfigToUI(HWND hwnd);
    void saveUIToConfig(HWND hwnd);
    
    // Actions
    void onPickRegion(HWND hwnd);
    void onTestSound(HWND hwnd);
    void onResetDefaults(HWND hwnd);
    void onBrowseSavePath(HWND hwnd);
    
    // Validation
    bool validateSettings(HWND hwnd);
    
    HWND m_parent;
    HWND m_hwnd = nullptr;
    HWND m_targetWindow;
    ExtendedConfig m_config;
    ExtendedConfig m_originalConfig;
    
    // Contrôles par onglet
    std::vector<HWND> m_detectionControls;
    std::vector<HWND> m_regionControls;
    std::vector<HWND> m_counterControls;
    std::vector<HWND> m_advancedControls;
    
    HWND m_tabControl = nullptr;
    int m_currentTab = 0;
    int m_initialTab = 0;
    int m_dialogResult = IDCANCEL;
    
    HINSTANCE m_hInstance;
    HBRUSH m_darkBrush = nullptr;
};

// Classe pour la sélection de région avec overlay
class RegionSelector {
public:
    RegionSelector(HWND targetWindow);
    ~RegionSelector();
    
    // Lancer la sélection (bloquant)
    // Retourne true si une région a été sélectionnée
    bool select();
    
    // Récupérer la région sélectionnée
    CaptureRegion getRegion() const { return m_region; }

private:
    static LRESULT CALLBACK OverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    void onPaint(HWND hwnd);
    void onMouseDown(HWND hwnd, int x, int y);
    void onMouseMove(HWND hwnd, int x, int y);
    void onMouseUp(HWND hwnd, int x, int y);
    
    HWND m_targetWindow;
    HWND m_overlayWindow = nullptr;
    CaptureRegion m_region;
    
    bool m_selecting = false;
    bool m_completed = false;
    bool m_cancelled = false;
    POINT m_startPoint = {0, 0};
    POINT m_currentPoint = {0, 0};
};
