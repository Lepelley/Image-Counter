# Image Counter

Application Windows native pour détecter automatiquement des images dans une fenêtre et incrémenter un compteur. Idéal pour compter des événements dans OBS, des jeux, ou toute autre application.

![Windows](https://img.shields.io/badge/Windows-10%2F11-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-orange)
![OpenCV](https://img.shields.io/badge/OpenCV-4.x-green)

## Fonctionnalités

### Détection
- ✅ Capture de n'importe quelle fenêtre Windows (même en arrière-plan)
- ✅ Détection d'image par template matching (OpenCV)
- ✅ Seuil de détection configurable (0-100%)
- ✅ Cooldown entre les détections (0-300 secondes)
- ✅ Sélection visuelle de zone de capture avec overlay
- ✅ Capture rapide depuis une région sélectionnée

### Multi-compteurs
- ✅ Onglets multiples pour gérer plusieurs compteurs simultanément
- ✅ Chaque compteur a sa propre configuration indépendante
- ✅ Renommage des compteurs via les paramètres

### Raccourcis clavier
- ✅ Raccourci global configurable pour incrémenter le compteur
- ✅ Fonctionne même quand l'application est en arrière-plan
- ✅ Chaque onglet peut avoir son propre raccourci

### Historique
- ✅ Journal complet des détections avec horodatage
- ✅ Export CSV de l'historique
- ✅ Affichage du score de confiance pour chaque détection

### Interface
- ✅ Thème sombre moderne
- ✅ Support multilingue (Français / English)
- ✅ Prévisualisation en temps réel de la capture
- ✅ Boutons +/- pour ajuster le compteur manuellement
- ✅ Notifications sonores configurables

### Persistance
- ✅ Sauvegarde automatique de tous les paramètres
- ✅ Restauration des onglets et configurations au démarrage
- ✅ Chemin de sauvegarde personnalisable

## Paramètres disponibles

### Onglet Détection
| Paramètre | Plage | Défaut | Description |
|-----------|-------|--------|-------------|
| Seuil de détection | 0-100% | 85% | Niveau minimum de correspondance |
| Intervalle de scan | 50-5000 ms | 500 ms | Temps entre chaque scan |
| Cooldown | 0-300000 ms | 1000 ms | Délai minimum entre deux détections |
| Détection multiple | On/Off | Off | Détecter plusieurs occurrences |

### Onglet Zone
- **Fenêtre entière** : Capture toute la zone client
- **Zone personnalisée** : Définir une région spécifique
  - Saisie manuelle (X, Y, Largeur, Hauteur)
  - Sélection visuelle avec overlay semi-transparent

### Onglet Compteur
| Paramètre | Description |
|-----------|-------------|
| Nom du compteur | Nom affiché dans l'onglet |
| Pas du compteur | Valeur ajoutée à chaque détection (1-1000) |
| Notification sonore | Activer/désactiver + choix du son |
| Raccourci clavier | Touche globale pour incrémenter manuellement |
| Chemin de sauvegarde | Emplacement du fichier de compteur |

### Onglet Avancé
| Paramètre | Options | Description |
|-----------|---------|-------------|
| Méthode de correspondance | TM_CCOEFF_NORMED, TM_CCORR_NORMED, TM_SQDIFF_NORMED | Algorithme OpenCV |
| Mode niveaux de gris | On/Off | Plus rapide mais moins précis |

## Installation

### Prérequis
- Windows 10/11
- Visual Studio 2019+ ou MinGW-w64
- CMake 3.16+
- OpenCV 4.x

### Installation d'OpenCV

#### Option 1: vcpkg (recommandé)

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

#### Option 2: Téléchargement direct

1. Télécharger OpenCV depuis https://opencv.org/releases/
2. Extraire dans `C:\opencv`
3. Ajouter `C:\opencv\build\x64\vc16\bin` au PATH
4. Définir `OpenCV_DIR=C:\opencv\build`

### Compilation

#### Avec Visual Studio

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

#### Avec MinGW

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Utilisation

### Démarrage rapide

1. **Lancer l'application**
2. **Sélectionner la fenêtre cible** dans la liste déroulante
3. **Charger une image de référence** (bouton "Charger" ou "Capture rapide")
4. **Optionnel** : Définir une zone de capture spécifique
5. **Cliquer sur Démarrer** pour lancer la détection
6. Le compteur s'incrémente automatiquement à chaque détection

### Capture rapide

Le bouton "Capture rapide" permet de :
1. Sélectionner visuellement une zone dans la fenêtre cible
2. Utiliser cette zone comme image de référence
3. Configurer automatiquement la région de détection

### Raccourcis clavier recommandés

| Raccourci | Usage |
|-----------|-------|
| F9-F12 | Simples, peu de conflits |
| Ctrl + ` | Intuitif |
| Numpad + | Pour incrémenter |

**À éviter** : F2-F5 (quicksave jeux), Alt+Tab, Ctrl+C/V/X/Z

## Architecture

```
ImageCounter/
├── src/
│   ├── ImageCounter.h      # Déclarations principales + structures
│   ├── ImageDetector.cpp   # Capture d'écran + détection OpenCV
│   ├── MainWindow.cpp      # Interface principale + gestion onglets
│   ├── SettingsDialog.h    # Déclarations boîte de dialogue
│   ├── SettingsDialog.cpp  # Paramètres + sélecteur de zone
│   ├── Localization.cpp    # Traductions FR/EN
│   ├── ThemeManager.cpp    # Thème sombre Windows
│   └── main.cpp            # Point d'entrée
├── CMakeLists.txt
├── app.manifest
└── README.md
```

## Fichiers de configuration

Les fichiers sont sauvegardés dans `Documents\ImageCounter\` :

| Fichier | Description |
|---------|-------------|
| `settings.ini` | Configuration globale (langue, thème, liste des onglets) |
| `<nom>_tab.ini` | Configuration de chaque onglet |
| `<nom>_ref.png` | Image de référence de chaque onglet |
| `<nom>.txt` | Valeur du compteur |
| `debug_log.txt` | Journal de débogage (si activé) |

## Algorithme de détection

L'application utilise `cv::matchTemplate` d'OpenCV :

- **TM_CCOEFF_NORMED** (défaut) : Corrélation croisée normalisée, robuste aux variations de luminosité
- **TM_CCORR_NORMED** : Corrélation standard normalisée
- **TM_SQDIFF_NORMED** : Différence au carré (inversé : minimum = meilleur match)

Le score est normalisé entre 0 et 1 (affiché en pourcentage).

## Dépannage

### PrintWindow échoue sur certaines applications
Certaines applications (jeux DirectX, apps protégées) peuvent bloquer la capture. L'application utilise automatiquement BitBlt comme fallback si la fenêtre est visible.

### Le compteur ne détecte pas l'image
- Vérifiez que le seuil n'est pas trop élevé (essayez 70-80%)
- Assurez-vous que l'image de référence correspond exactement à ce qui apparaît
- Utilisez "Capture rapide" pour capturer directement depuis la fenêtre cible


## Licence

MIT License
