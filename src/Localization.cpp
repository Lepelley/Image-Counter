#include "ImageCounter.h"

Localization::Localization() {
    // === FRANÇAIS ===
    
    // Titre
    m_french[L"app_title"] = L"Image Counter - Détecteur d'images";
    
    // Onglets
    m_french[L"counter"] = L"Compteur";
    m_french[L"new_counter"] = L"Nouveau";
    
    // Fenêtre cible
    m_french[L"target_window"] = L"Fenêtre cible:";
    
    // Boutons principaux
    m_french[L"load_image"] = L"Charger image...";
    m_french[L"quick_capture"] = L"Capture rapide";
    m_french[L"select_zone"] = L"Sélectionner zone";
    m_french[L"settings"] = L"Paramètres...";
    m_french[L"start"] = L"Démarrer";
    m_french[L"stop"] = L"Arrêter";
    m_french[L"reset"] = L"Réinitialiser";
    
    // Labels
    m_french[L"counter_label"] = L"Compteur:";
    m_french[L"status"] = L"Statut:";
    m_french[L"status_waiting"] = L"En attente";
    m_french[L"status_ready"] = L"Prêt";
    m_french[L"status_scanning"] = L"Scan en cours...";
    m_french[L"status_stopped"] = L"Arrêté";
    m_french[L"status_image_loaded"] = L"Image chargée";
    m_french[L"status_window_selected"] = L"Fenêtre sélectionnée";
    m_french[L"status_waiting_reference"] = L"En attente d'une image de référence";
    m_french[L"reference_image"] = L"Image de référence:";
    m_french[L"current_capture"] = L"Capture en cours:";
    
    // Dialogues
    m_french[L"rename_tab"] = L"Renommer l'onglet";
    m_french[L"new_name"] = L"Nouveau nom:";
    m_french[L"ok"] = L"OK";
    m_french[L"cancel"] = L"Annuler";
    m_french[L"apply"] = L"Appliquer";
    m_french[L"default"] = L"Par défaut";
    m_french[L"error"] = L"Erreur";
    m_french[L"warning"] = L"Attention";
    m_french[L"validation"] = L"Validation";
    
    // Messages d'erreur
    m_french[L"error_load_image"] = L"Impossible de charger l'image.";
    m_french[L"error_no_reference"] = L"Veuillez d'abord charger une image de référence.";
    m_french[L"error_no_window"] = L"Veuillez d'abord sélectionner une fenêtre cible.";
    m_french[L"error_need_one_tab"] = L"Vous devez garder au moins un onglet.";
    m_french[L"error_scan_interval"] = L"L'intervalle de scan doit être entre 0.05 et 5 secondes.";
    m_french[L"error_cooldown"] = L"Le cooldown doit être entre 0 et 300 secondes.";
    
    // Thème
    m_french[L"theme"] = L"Thème:";
    m_french[L"theme_system"] = L"Système";
    m_french[L"theme_light"] = L"Clair";
    m_french[L"theme_dark"] = L"Sombre";
    
    // Langue
    m_french[L"language"] = L"Langue:";
    
    // Paramètres - Titre
    m_french[L"settings_title"] = L"Paramètres";
    
    // Paramètres - Onglets
    m_french[L"tab_detection"] = L"Détection";
    m_french[L"tab_region"] = L"Zone";
    m_french[L"tab_counter"] = L"Compteur";
    m_french[L"tab_advanced"] = L"Avancé";
    
    // Paramètres - Détection
    m_french[L"threshold"] = L"Seuil de détection:";
    m_french[L"scan_interval"] = L"Intervalle de scan:";
    m_french[L"cooldown"] = L"Délai entre détections:";
    m_french[L"detect_multiple"] = L"Détecter plusieurs occurrences";
    m_french[L"detection_info"] = L"Le cooldown empêche de compter plusieurs fois\r\nla même apparition d'image.";
    m_french[L"seconds_short"] = L"s (0.05 - 5)";
    m_french[L"seconds_cooldown"] = L"s (0 - 300)";
    m_french[L"status_settings_updated"] = L"Statut: Paramètres mis à jour";
    m_french[L"status_defaults_reset"] = L"Paramètres réinitialisés aux valeurs par défaut.";
    
    // Paramètres - Zone
    m_french[L"capture_region"] = L"Zone de capture";
    m_french[L"full_window"] = L"Fenêtre entière";
    m_french[L"custom_region"] = L"Zone personnalisée:";
    m_french[L"pick_region"] = L"Sélectionner visuellement...";
    m_french[L"region_x"] = L"X:";
    m_french[L"region_y"] = L"Y:";
    m_french[L"region_w"] = L"Largeur:";
    m_french[L"region_h"] = L"Hauteur:";
    m_french[L"region_preview"] = L"Aperçu de la zone:";
    
    // Paramètres - Compteur
    m_french[L"counter_value"] = L"Valeur actuelle du compteur:";
    m_french[L"counter_step"] = L"Pas du compteur:";
    m_french[L"sound_section"] = L"Son";
    m_french[L"sound_enabled"] = L"Jouer un son à chaque détection";
    m_french[L"sound_type"] = L"Son:";
    m_french[L"sound_beep"] = L"Beep système";
    m_french[L"sound_ding"] = L"Ding";
    m_french[L"sound_notification"] = L"Notification Windows";
    m_french[L"test_sound"] = L"Test";
    m_french[L"save_section"] = L"Sauvegarde";
    m_french[L"auto_save"] = L"Sauvegarder automatiquement le compteur";
    m_french[L"auto_save_info"] = L"La sauvegarde automatique conserve la valeur\r\ndu compteur entre les sessions.";
    m_french[L"save_path"] = L"Sauvegarder le fichier dans:";
    
    // Paramètres - Avancé
    m_french[L"match_method"] = L"Méthode de correspondance:";
    m_french[L"use_grayscale"] = L"Convertir en niveaux de gris (plus rapide)";
    m_french[L"debug_mode"] = L"Mode debug (afficher les résultats de détection)";
    m_french[L"appearance_section"] = L"Apparence";
    m_french[L"export_log"] = L"Exporter le journal...";
    
    // Dialog modifier compteur
    m_french[L"edit_counter_title"] = L"Modifier le compteur";
    m_french[L"new_value"] = L"Nouvelle valeur:";
    
    // Historique
    m_french[L"history"] = L"Historique";
    m_french[L"export_csv"] = L"Exporter CSV...";
    m_french[L"clear_history"] = L"Effacer";
    m_french[L"close"] = L"Fermer";
    m_french[L"history_empty"] = L"L'historique est vide.";
    m_french[L"confirm_clear_history"] = L"Voulez-vous vraiment effacer tout l'historique ?";
    m_french[L"confirmation"] = L"Confirmation";
    
    // Raccourci clavier
    m_french[L"hotkey_increment"] = L"Raccourci (+pas):";
    m_french[L"clear"] = L"Effacer";
    
    // Nom du compteur
    m_french[L"counter_name"] = L"Nom du compteur:";

    // === ANGLAIS ===
    
    // Title
    m_english[L"app_title"] = L"Image Counter - Image Detector";
    
    // Tabs
    m_english[L"counter"] = L"Counter";
    m_english[L"new_counter"] = L"New";
    
    // Target window
    m_english[L"target_window"] = L"Target window:";
    
    // Main buttons
    m_english[L"load_image"] = L"Load image...";
    m_english[L"quick_capture"] = L"Quick capture";
    m_english[L"select_zone"] = L"Select zone";
    m_english[L"settings"] = L"Settings...";
    m_english[L"start"] = L"Start";
    m_english[L"stop"] = L"Stop";
    m_english[L"reset"] = L"Reset";
    
    // Labels
    m_english[L"counter_label"] = L"Counter:";
    m_english[L"status"] = L"Status:";
    m_english[L"status_waiting"] = L"Waiting";
    m_english[L"status_ready"] = L"Ready";
    m_english[L"status_scanning"] = L"Scanning...";
    m_english[L"status_stopped"] = L"Stopped";
    m_english[L"status_image_loaded"] = L"Image loaded";
    m_english[L"status_window_selected"] = L"Window selected";
    m_english[L"status_waiting_reference"] = L"Waiting for reference image";
    m_english[L"reference_image"] = L"Reference image:";
    m_english[L"current_capture"] = L"Current capture:";
    
    // Dialogs
    m_english[L"rename_tab"] = L"Rename tab";
    m_english[L"new_name"] = L"New name:";
    m_english[L"ok"] = L"OK";
    m_english[L"cancel"] = L"Cancel";
    m_english[L"apply"] = L"Apply";
    m_english[L"default"] = L"Default";
    m_english[L"error"] = L"Error";
    m_english[L"warning"] = L"Warning";
    m_english[L"validation"] = L"Validation";
    
    // Error messages
    m_english[L"error_load_image"] = L"Unable to load image.";
    m_english[L"error_no_reference"] = L"Please load a reference image first.";
    m_english[L"error_no_window"] = L"Please select a target window first.";
    m_english[L"error_need_one_tab"] = L"You must keep at least one tab.";
    m_english[L"error_scan_interval"] = L"Scan interval must be between 0.05 and 5 seconds.";
    m_english[L"error_cooldown"] = L"Cooldown must be between 0 and 300 seconds.";
    
    // Theme
    m_english[L"theme"] = L"Theme:";
    m_english[L"theme_system"] = L"System";
    m_english[L"theme_light"] = L"Light";
    m_english[L"theme_dark"] = L"Dark";
    
    // Language
    m_english[L"language"] = L"Language:";
    
    // Settings - Title
    m_english[L"settings_title"] = L"Settings";
    
    // Settings - Tabs
    m_english[L"tab_detection"] = L"Detection";
    m_english[L"tab_region"] = L"Region";
    m_english[L"tab_counter"] = L"Counter";
    m_english[L"tab_advanced"] = L"Advanced";
    
    // Settings - Detection
    m_english[L"threshold"] = L"Detection threshold:";
    m_english[L"scan_interval"] = L"Scan interval:";
    m_english[L"cooldown"] = L"Cooldown between detections:";
    m_english[L"detect_multiple"] = L"Detect multiple occurrences";
    m_english[L"detection_info"] = L"The cooldown prevents counting the same\r\nimage appearance multiple times.";
    m_english[L"status_settings_updated"] = L"Status: Settings updated";
    m_english[L"status_defaults_reset"] = L"Settings reset to default values.";
    m_english[L"seconds_short"] = L"s (0.05 - 5)";
    m_english[L"seconds_cooldown"] = L"s (0 - 300)";
    
    // Settings - Region
    m_english[L"capture_region"] = L"Capture region";
    m_english[L"full_window"] = L"Full window";
    m_english[L"custom_region"] = L"Custom region:";
    m_english[L"pick_region"] = L"Select visually...";
    m_english[L"region_x"] = L"X:";
    m_english[L"region_y"] = L"Y:";
    m_english[L"region_w"] = L"Width:";
    m_english[L"region_h"] = L"Height:";
    m_english[L"region_preview"] = L"Region preview:";
    
    // Settings - Counter
    m_english[L"counter_value"] = L"Current counter value:";
    m_english[L"counter_step"] = L"Counter step:";
    m_english[L"sound_section"] = L"Sound";
    m_english[L"sound_enabled"] = L"Play sound on each detection";
    m_english[L"sound_type"] = L"Sound:";
    m_english[L"sound_beep"] = L"System beep";
    m_english[L"sound_ding"] = L"Ding";
    m_english[L"sound_notification"] = L"Windows notification";
    m_english[L"test_sound"] = L"Test";
    m_english[L"save_section"] = L"Save";
    m_english[L"auto_save"] = L"Automatically save counter";
    m_english[L"auto_save_info"] = L"Auto-save preserves the counter value\r\nbetween sessions.";
    m_english[L"save_path"] = L"Save file to:";
    
    // Settings - Advanced
    m_english[L"match_method"] = L"Match method:";
    m_english[L"use_grayscale"] = L"Convert to grayscale (faster)";
    m_english[L"debug_mode"] = L"Debug mode (show detection results)";
    m_english[L"appearance_section"] = L"Appearance";
    m_english[L"export_log"] = L"Export log...";
    
    // Edit counter dialog
    m_english[L"edit_counter_title"] = L"Edit counter";
    m_english[L"new_value"] = L"New value:";
    
    // History
    m_english[L"history"] = L"History";
    m_english[L"export_csv"] = L"Export CSV...";
    m_english[L"clear_history"] = L"Clear";
    m_english[L"close"] = L"Close";
    m_english[L"history_empty"] = L"History is empty.";
    m_english[L"confirm_clear_history"] = L"Do you really want to clear all history?";
    m_english[L"confirmation"] = L"Confirmation";
    
    // Hotkey
    m_english[L"hotkey_increment"] = L"Hotkey (+step):";
    m_english[L"clear"] = L"Clear";
    
    // Counter name
    m_english[L"counter_name"] = L"Counter name:";
}
