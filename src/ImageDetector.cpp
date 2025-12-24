#include "ImageCounter.h"
#include <dwmapi.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include <shlobj.h>

#pragma comment(lib, "dwmapi.lib")

// ============================================================================
// Debug Logging (simple version)
// ============================================================================

static std::mutex g_logMutex;
static std::string g_logPath;
static bool g_logInitialized = false;

static void initLog() {
    if (g_logInitialized) return;
    
    wchar_t documentsPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_MYDOCUMENTS, nullptr, 0, documentsPath))) {
        std::wstring wpath = documentsPath;
        wpath += L"\\ImageCounter\\debug_log.txt";
        
        // Convert to narrow string
        char narrowPath[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, narrowPath, MAX_PATH, nullptr, nullptr);
        g_logPath = narrowPath;
        
        // Clear existing log
        std::ofstream clear(g_logPath, std::ios::trunc);
        clear.close();
    }
    g_logInitialized = true;
}

static void writeLog(const char* message) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    if (!g_logInitialized) initLog();
    if (g_logPath.empty()) return;
    
    std::ofstream file(g_logPath, std::ios::app);
    if (file.is_open()) {
        // Get timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm;
        localtime_s(&tm, &time);
        
        char timestamp[32];
        snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d.%03d", 
            tm.tm_hour, tm.tm_min, tm.tm_sec, (int)ms.count());
        
        file << timestamp << " | " << message << std::endl;
    }
}

#define LOG(msg) writeLog(msg)

// ============================================================================
// ImageDetector Implementation
// ============================================================================

ImageDetector::ImageDetector() {}

ImageDetector::~ImageDetector() {
    stopScanning();
}

bool ImageDetector::loadReferenceImage(const std::wstring& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Convertir wstring en string pour OpenCV
    int size = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8Path(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &utf8Path[0], size, nullptr, nullptr);
    
    m_referenceImage = cv::imread(utf8Path, cv::IMREAD_COLOR);
    return !m_referenceImage.empty();
}

bool ImageDetector::setReferenceFromClipboard() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    
    bool success = false;
    HANDLE hBitmap = GetClipboardData(CF_BITMAP);
    
    if (hBitmap) {
        HBITMAP hBmp = (HBITMAP)hBitmap;
        BITMAP bmp;
        GetObject(hBmp, sizeof(BITMAP), &bmp);
        
        // Créer un DC compatible
        HDC hdcScreen = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        
        BITMAPINFOHEADER bi = {};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = bmp.bmWidth;
        bi.biHeight = -bmp.bmHeight; // Top-down
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;
        
        m_referenceImage = cv::Mat(bmp.bmHeight, bmp.bmWidth, CV_8UC4);
        
        GetDIBits(hdcMem, hBmp, 0, bmp.bmHeight, m_referenceImage.data, 
                  (BITMAPINFO*)&bi, DIB_RGB_COLORS);
        
        cv::cvtColor(m_referenceImage, m_referenceImage, cv::COLOR_BGRA2BGR);
        
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
        
        success = !m_referenceImage.empty();
    }
    
    CloseClipboard();
    return success;
}

void ImageDetector::setReferenceFromMat(const cv::Mat& image) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_referenceImage = image.clone();
}

void ImageDetector::clearReferenceImage() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_referenceImage.release();
}

bool ImageDetector::hasReferenceImage() const {
    return !m_referenceImage.empty();
}

cv::Mat ImageDetector::getReferenceImage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_referenceImage.clone();
}

bool ImageDetector::setTargetWindow(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        return false;
    }
    m_targetWindow = hwnd;
    return true;
}

bool ImageDetector::setTargetByTitle(const std::wstring& title) {
    HWND hwnd = FindWindowW(nullptr, title.c_str());
    if (hwnd) {
        m_targetWindow = hwnd;
        return true;
    }
    return false;
}

HWND ImageDetector::getTargetWindow() const {
    return m_targetWindow;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    auto* windows = reinterpret_cast<std::vector<std::pair<HWND, std::wstring>>*>(lParam);
    
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }
    
    int length = GetWindowTextLengthW(hwnd);
    if (length == 0) {
        return TRUE;
    }
    
    std::wstring title(length + 1, L'\0');
    GetWindowTextW(hwnd, &title[0], length + 1);
    title.resize(length);
    
    // Filtrer les fenêtres sans titre significatif
    if (!title.empty()) {
        windows->push_back({hwnd, title});
    }
    
    return TRUE;
}

std::vector<std::pair<HWND, std::wstring>> ImageDetector::enumerateWindows() {
    std::vector<std::pair<HWND, std::wstring>> windows;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));
    return windows;
}

void ImageDetector::setConfig(const DetectionConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

DetectionConfig ImageDetector::getConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void ImageDetector::setCaptureRegion(const CaptureRegion& region) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_captureRegion = region;
}

CaptureRegion ImageDetector::getCaptureRegion() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    return m_captureRegion;
}

cv::Mat ImageDetector::hwndToMat(HWND hwnd) {
    static int captureCount = 0;
    captureCount++;
    
    // Log toutes les 100 captures ou en cas de problème
    bool shouldLog = (captureCount % 100 == 0);
    
    if (!IsWindow(hwnd)) {
        if (shouldLog) LOG("[hwndToMat] Window invalid");
        return cv::Mat();
    }
    
    if (IsIconic(hwnd)) {
        if (shouldLog) LOG("[hwndToMat] Window minimized");
        return cv::Mat();
    }
    
    // Vérifier si la fenêtre répond (timeout de 50ms)
    DWORD_PTR msgResult = 0;
    LRESULT sendResult = SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 50, &msgResult);
    if (!sendResult) {
        LOG("[hwndToMat] Window not responding");
        return cv::Mat();
    }
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    
    if (width <= 0 || height <= 0) {
        if (shouldLog) LOG("[hwndToMat] Invalid dimensions");
        return cv::Mat();
    }
    
    // Préparer les structures
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    
    cv::Mat mat(height, width, CV_8UC4);
    bool success = false;
    const char* methodUsed = "none";
    double brightness = 0;
    
    // Utiliser PrintWindow - méthode la plus fiable pour OBS et apps modernes
    HDC hdcWindow = GetDC(hwnd);
    if (hdcWindow) {
        HDC hdcMem = CreateCompatibleDC(hdcWindow);
        if (hdcMem) {
            HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
            if (hBitmap) {
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                
                // Essayer PrintWindow avec rendu complet
                BOOL printResult = PrintWindow(hwnd, hdcMem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
                if (printResult) {
                    GetDIBits(hdcMem, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    success = true;
                    methodUsed = "PrintWindow";
                } else {
                    LOG("[hwndToMat] PrintWindow failed");
                }
                
                SelectObject(hdcMem, hOldBitmap);
                DeleteObject(hBitmap);
            } else {
                LOG("[hwndToMat] CreateCompatibleBitmap failed");
            }
            DeleteDC(hdcMem);
        } else {
            LOG("[hwndToMat] CreateCompatibleDC failed");
        }
        ReleaseDC(hwnd, hdcWindow);
    } else {
        LOG("[hwndToMat] GetDC failed");
    }
    
    // Fallback: BitBlt depuis l'écran si PrintWindow échoue
    if (!success && IsWindowVisible(hwnd)) {
        POINT clientOrigin = {0, 0};
        ClientToScreen(hwnd, &clientOrigin);
        
        HDC hdcScreen = GetDC(nullptr);
        if (hdcScreen) {
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            if (hdcMem) {
                HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
                if (hBitmap) {
                    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                    
                    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, clientOrigin.x, clientOrigin.y, SRCCOPY);
                    GetDIBits(hdcMem, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    success = true;
                    methodUsed = "BitBlt";
                    
                    SelectObject(hdcMem, hOldBitmap);
                    DeleteObject(hBitmap);
                }
                DeleteDC(hdcMem);
            }
            ReleaseDC(nullptr, hdcScreen);
        }
    }
    
    if (!success) {
        LOG("[hwndToMat] All capture methods failed");
        return cv::Mat();
    }
    
    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_BGRA2BGR);
    
    // Calculer la luminosité moyenne pour détecter les images noires
    cv::Scalar mean = cv::mean(result);
    brightness = (mean[0] + mean[1] + mean[2]) / 3.0;
    
    // Logger si image très sombre ou périodiquement
    char logBuf[256];
    if (brightness < 5.0) {
        snprintf(logBuf, sizeof(logBuf), "[hwndToMat] BLACK IMAGE! method=%s brightness=%.1f size=%dx%d", 
            methodUsed, brightness, width, height);
        LOG(logBuf);
    } else if (shouldLog) {
        snprintf(logBuf, sizeof(logBuf), "[hwndToMat] OK method=%s brightness=%.0f size=%dx%d count=%d", 
            methodUsed, brightness, width, height, captureCount);
        LOG(logBuf);
    }
    
    return result;
}

cv::Mat ImageDetector::hwndToMat(HWND hwnd, const CaptureRegion& region) {
    cv::Mat fullCapture = hwndToMat(hwnd);
    
    if (fullCapture.empty() || region.useFullWindow) {
        return fullCapture;
    }
    
    // Vérifier les limites
    int x = std::max(0, region.x);
    int y = std::max(0, region.y);
    int w = std::min(region.width, fullCapture.cols - x);
    int h = std::min(region.height, fullCapture.rows - y);
    
    if (w <= 0 || h <= 0) {
        return fullCapture;
    }
    
    return fullCapture(cv::Rect(x, y, w, h)).clone();
}

cv::Mat ImageDetector::captureWindow() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return hwndToMat(m_targetWindow);
}

cv::Mat ImageDetector::captureRegion() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return hwndToMat(m_targetWindow, m_captureRegion);
}

DetectionResult ImageDetector::detectImage(const cv::Mat& source) {
    DetectionResult result;
    result.timestamp = GetTickCount64();
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (source.empty() || m_referenceImage.empty()) {
        return result;
    }
    
    // L'image de référence doit être plus petite que la source
    if (m_referenceImage.cols > source.cols || m_referenceImage.rows > source.rows) {
        return result;
    }
    
    cv::Mat sourceToUse = source;
    cv::Mat refToUse = m_referenceImage;
    
    // Convertir en niveaux de gris si demandé
    if (m_config.useGrayscale) {
        if (source.channels() > 1) {
            cv::cvtColor(source, sourceToUse, cv::COLOR_BGR2GRAY);
        }
        if (m_referenceImage.channels() > 1) {
            cv::cvtColor(m_referenceImage, refToUse, cv::COLOR_BGR2GRAY);
        }
    }
    
    // Choisir la méthode de matching
    int method;
    switch (m_config.matchMethod) {
        case 1: method = cv::TM_CCORR_NORMED; break;
        case 2: method = cv::TM_SQDIFF_NORMED; break;
        default: method = cv::TM_CCOEFF_NORMED; break;
    }
    
    cv::Mat resultMat;
    cv::matchTemplate(sourceToUse, refToUse, resultMat, method);
    
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(resultMat, &minVal, &maxVal, &minLoc, &maxLoc);
    
    // Pour TM_SQDIFF_NORMED, le minimum est le meilleur match
    if (method == cv::TM_SQDIFF_NORMED) {
        result.confidence = 1.0 - minVal;
        result.location = {minLoc.x, minLoc.y};
    } else {
        result.confidence = maxVal;
        result.location = {maxLoc.x, maxLoc.y};
    }
    
    result.found = (result.confidence >= m_config.threshold);
    
    return result;
}

void ImageDetector::startScanning() {
    if (m_scanning.exchange(true)) {
        return; // Déjà en cours
    }
    LOG("[scanThread] Starting");
    m_scanThread = std::thread(&ImageDetector::scanThread, this);
}

void ImageDetector::stopScanning() {
    LOG("[scanThread] Stopping");
    m_scanning = false;
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
    LOG("[scanThread] Stopped");
}

bool ImageDetector::isScanning() const {
    return m_scanning;
}

void ImageDetector::scanThread() {
    int consecutiveFailures = 0;
    const int MAX_FAILURES = 10;
    
    LOG("[scanThread] Thread started");
    
    while (m_scanning) {
        // Vérifier que la fenêtre cible est toujours valide
        HWND target = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            target = m_targetWindow;
        }
        
        if (!target || !IsWindow(target)) {
            consecutiveFailures++;
            if (consecutiveFailures == 1 || consecutiveFailures == MAX_FAILURES) {
                char buf[128];
                snprintf(buf, sizeof(buf), "[scanThread] Target window invalid, failures=%d", consecutiveFailures);
                LOG(buf);
            }
            if (consecutiveFailures >= MAX_FAILURES) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(m_config.scanIntervalMs));
            }
            continue;
        }
        
        cv::Mat capture = captureRegion();
        
        if (!capture.empty()) {
            consecutiveFailures = 0;
            
            DetectionResult result = detectImage(capture);
            
            // Toujours mettre à jour le dernier score de confiance
            m_lastConfidence = result.confidence;
            
            if (result.found) {
                ULONGLONG currentTime = GetTickCount64();
                
                // Vérifier le cooldown
                if (currentTime - m_lastDetectionTime >= static_cast<ULONGLONG>(m_config.cooldownMs)) {
                    m_lastDetectionTime = currentTime;
                    m_counter += m_config.counterStep;
                    int newCounter = m_counter;
                    
                    char buf[128];
                    snprintf(buf, sizeof(buf), "[scanThread] DETECTION! counter=%d confidence=%d%%", 
                        newCounter, (int)(result.confidence * 100));
                    LOG(buf);
                    
                    if (m_callback) {
                        m_callback(result, newCounter);
                    }
                }
            }
        } else {
            consecutiveFailures++;
            if (consecutiveFailures == 1 || consecutiveFailures == MAX_FAILURES) {
                char buf[128];
                snprintf(buf, sizeof(buf), "[scanThread] Capture empty, failures=%d", consecutiveFailures);
                LOG(buf);
            }
            if (consecutiveFailures >= MAX_FAILURES) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.scanIntervalMs));
    }
    
    LOG("[scanThread] Thread exiting");
}

int ImageDetector::getCounter() const {
    return m_counter;
}

void ImageDetector::resetCounter() {
    m_counter = 0;
}

void ImageDetector::setCounter(int value) {
    m_counter = value;
}

void ImageDetector::setDetectionCallback(DetectionCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(callback);
}
