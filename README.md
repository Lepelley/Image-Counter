# Image Counter

Application Windows native pour détecter automatiquement des images dans une fenêtre (OBS, etc.) et incrémenter un compteur.

## Fonctionnalités

- ✅ Capture d'écran de n'importe quelle fenêtre Windows
- ✅ Détection d'image par template matching (OpenCV)
- ✅ Seuil de détection configurable
- ✅ Cooldown entre les détections pour éviter les doublons
- ✅ Interface graphique Win32 native
- ✅ Prévisualisation de l'image de référence
- ✅ Sélection visuelle de zone de capture (overlay)
- ✅ Boîte de dialogue de paramètres complète avec onglets
- ✅ Notification sonore à chaque détection
- ✅ Choix de la méthode de correspondance OpenCV
- ✅ Mode niveaux de gris (optimisation)
- 🔄 Sauvegarde automatique de la configuration
- 🔄 Export du journal des détections

## Paramètres disponibles

### Onglet Détection
- **Seuil de détection** (0-100%): Niveau minimum de correspondance
- **Intervalle de scan** (50-5000 ms): Temps entre chaque scan
- **Cooldown** (0-10000 ms): Délai minimum entre deux détections
- **Détection multiple**: Détecter plusieurs occurrences dans une image

### Onglet Zone
- **Fenêtre entière** ou **Zone personnalisée**
- Saisie manuelle des coordonnées (X, Y, Largeur, Hauteur)
- **Sélection visuelle** avec overlay semi-transparent

### Onglet Compteur
- Modifier la valeur du compteur manuellement
- **Notification sonore** avec choix du son
- Sauvegarde automatique entre les sessions

### Onglet Avancé
- **Méthode de correspondance**: TM_CCOEFF_NORMED, TM_CCORR_NORMED, TM_SQDIFF_NORMED
- **Mode niveaux de gris**: Plus rapide mais moins précis
- **Mode debug**: Afficher les résultats de détection

## Prérequis

- Windows 10/11
- Visual Studio 2019+ ou MinGW-w64
- CMake 3.16+
- OpenCV 4.x

## Installation d'OpenCV

### Option 1: vcpkg (recommandé)

```bash
# Installer vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Installer OpenCV
.\vcpkg install opencv4:x64-windows

# Intégrer avec Visual Studio
.\vcpkg integrate install
```

### Option 2: Téléchargement direct

1. Télécharger OpenCV depuis https://opencv.org/releases/
2. Extraire dans `C:\opencv`
3. Ajouter `C:\opencv\build\x64\vc16\bin` au PATH
4. Définir `OpenCV_DIR=C:\opencv\build`

## Compilation

### Avec Visual Studio

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Avec MinGW

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Utilisation

1. **Lancer l'application**
2. **Sélectionner la fenêtre cible** dans la liste déroulante (ex: "Projecteur OBS")
3. **Charger une image de référence** - l'image à détecter
4. **Cliquer sur Démarrer** pour lancer le scan
5. Le compteur s'incrémente à chaque détection

## Configuration

| Paramètre | Défaut | Description |
|-----------|--------|-------------|
| Seuil | 85% | Niveau minimum de correspondance |
| Intervalle | 500ms | Temps entre chaque scan |
| Cooldown | 1000ms | Délai minimum entre deux détections |

## Architecture

```
ImageCounter/
├── src/
│   ├── ImageCounter.h      # Déclarations principales
│   ├── ImageDetector.cpp   # Logique de détection (OpenCV)
│   ├── MainWindow.cpp      # Interface Win32 principale
│   ├── SettingsDialog.h    # Déclarations boîte de dialogue
│   ├── SettingsDialog.cpp  # Paramètres + sélecteur de zone
│   └── main.cpp            # Point d'entrée
├── CMakeLists.txt
├── app.manifest
└── README.md
```

## Algorithme de détection

L'application utilise `cv::matchTemplate` avec la méthode `TM_CCOEFF_NORMED` qui:
- Calcule un score de correspondance normalisé entre 0 et 1
- Est robuste aux variations de luminosité
- Trouve la meilleure position de correspondance dans l'image source

## Licence

MIT License
