#include "ImageCounter.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

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
    if (!IsWindow(hwnd)) {
        return cv::Mat();
    }
    
    // Vérifier si la fenêtre est minimisée
    if (IsIconic(hwnd)) {
        return cv::Mat();
    }
    
    // Vérifier si la fenêtre répond (timeout de 50ms pour être plus réactif)
    DWORD_PTR msgResult = 0;
    if (!SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 50, &msgResult)) {
        return cv::Mat();
    }
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    
    if (width <= 0 || height <= 0) {
        return cv::Mat();
    }
    
    // Préparer les structures pour la capture
    BITMAPINFOHEADER bi = {};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Top-down
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    
    cv::Mat mat(height, width, CV_8UC4);
    
    // Méthode 1: Essayer BitBlt depuis le DC de la fenêtre (rapide, fonctionne pour fenêtres au premier plan)
    bool captureSuccess = false;
    
    if (IsWindowVisible(hwnd)) {
        // Vérifier si la fenêtre est au premier plan ou partiellement visible
        RECT windowRect;
        GetWindowRect(hwnd, &windowRect);
        
        POINT clientOrigin = {0, 0};
        ClientToScreen(hwnd, &clientOrigin);
        
        HDC hdcScreen = GetDC(nullptr);
        if (hdcScreen) {
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            if (hdcMem) {
                HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
                if (hBitmap) {
                    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                    
                    // Capturer depuis l'écran à la position de la fenêtre
                    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, clientOrigin.x, clientOrigin.y, SRCCOPY);
                    GetDIBits(hdcMem, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    
                    SelectObject(hdcMem, hOldBitmap);
                    DeleteObject(hBitmap);
                    
                    // Vérifier si la capture est valide (pas noire)
                    cv::Mat gray;
                    cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);
                    double minVal, maxVal;
                    cv::minMaxLoc(gray, &minVal, &maxVal);
                    captureSuccess = (maxVal > 15);
                }
                DeleteDC(hdcMem);
            }
            ReleaseDC(nullptr, hdcScreen);
        }
    }
    
    // Méthode 2: Si BitBlt échoue, essayer PrintWindow
    if (!captureSuccess) {
        HDC hdcWindow = GetDC(hwnd);
        if (hdcWindow) {
            HDC hdcMem = CreateCompatibleDC(hdcWindow);
            if (hdcMem) {
                HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
                if (hBitmap) {
                    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                    
                    // PrintWindow avec PW_RENDERFULLCONTENT pour les fenêtres modernes
                    PrintWindow(hwnd, hdcMem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
                    GetDIBits(hdcMem, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    
                    SelectObject(hdcMem, hOldBitmap);
                    DeleteObject(hBitmap);
                    
                    // Vérifier si la capture est valide
                    cv::Mat gray;
                    cv::cvtColor(mat, gray, cv::COLOR_BGRA2GRAY);
                    double minVal, maxVal;
                    cv::minMaxLoc(gray, &minVal, &maxVal);
                    captureSuccess = (maxVal > 15);
                }
                DeleteDC(hdcMem);
            }
            ReleaseDC(hwnd, hdcWindow);
        }
    }
    
    // Méthode 3: PrintWindow sans PW_RENDERFULLCONTENT (fallback pour anciennes apps)
    if (!captureSuccess) {
        HDC hdcWindow = GetDC(hwnd);
        if (hdcWindow) {
            HDC hdcMem = CreateCompatibleDC(hdcWindow);
            if (hdcMem) {
                HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
                if (hBitmap) {
                    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                    
                    PrintWindow(hwnd, hdcMem, PW_CLIENTONLY);
                    GetDIBits(hdcMem, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    
                    SelectObject(hdcMem, hOldBitmap);
                    DeleteObject(hBitmap);
                }
                DeleteDC(hdcMem);
            }
            ReleaseDC(hwnd, hdcWindow);
        }
    }
    
    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_BGRA2BGR);
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
    
    m_scanThread = std::thread(&ImageDetector::scanThread, this);
}

void ImageDetector::stopScanning() {
    m_scanning = false;
    if (m_scanThread.joinable()) {
        m_scanThread.join();
    }
}

bool ImageDetector::isScanning() const {
    return m_scanning;
}

void ImageDetector::scanThread() {
    while (m_scanning) {
        cv::Mat capture = captureRegion();
        
        if (!capture.empty()) {
            DetectionResult result = detectImage(capture);
            
            // Toujours mettre à jour le dernier score de confiance
            m_lastConfidence = result.confidence;
            
            if (result.found) {
                ULONGLONG currentTime = GetTickCount64();
                
                // Vérifier le cooldown
                if (currentTime - m_lastDetectionTime >= static_cast<ULONGLONG>(m_config.cooldownMs)) {
                    m_lastDetectionTime = currentTime;
                    m_counter += m_config.counterStep;  // Utiliser le pas configuré
                    int newCounter = m_counter;
                    
                    if (m_callback) {
                        m_callback(result, newCounter);
                    }
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(m_config.scanIntervalMs));
    }
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
