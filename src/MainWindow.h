/*!
 * ********************************************************************************************************************************
 * @file         MainWindow.h
 * @brief        Declares the main application window for ProjectSource.
 * @details      MainWindow owns and orchestrates the entire Qt Widgets UI for the ProjectSource
 * application.
 *
 *               Responsibilities include:
 *               - File menu (Open project, Open Recent, Exit)
 *               - Tools menu (Create Project‑Source.txt)
 *               - Help menu (Usage, About)
 *               - Central tabs (Log + Settings)
 *
 *               The Settings tab supports:
 *               - Editable project folder list (per‑project selection)
 *               - Include file extensions (comma/space/semicolon separated)
 *               - Exclude folder patterns with browse support
 *               - Backup base folder selection with timestamped backups
 *
 *               All per‑project settings are persisted using QSettings and applied when generating
 * Project‑Source.txt and performing project backups.
 *
 * @author       Jeffrey Scott Flesher
 * @date         2026-03-31
 *********************************************************************************************************************************/
#pragma once

#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QStringList>

/* Forward declarations */
class QAction;
class QListWidget;
class QMenu;
class QProgressBar;
class QTabWidget;
class QTextEdit;
class QToolBar;
class StatusBarQueue;
class QActionGroup;


static constexpr const char *kSettingsThemeMode = "ui/themeMode";

/*!
 * ********************************************************************************************************************************
 * @brief  Theme Mode
 *********************************************************************************************************************************/
enum class ThemeMode
{
    System,
    Light,
    Dark
};

/*!
 * ********************************************************************************************************************************
 * @class        MainWindow
 * @brief        Primary GUI window for the ProjectSource application.
 *
 * @details      MainWindow is responsible for all user‑facing interactions and delegates project
 * source generation and backup logic to ProjectSourceExporter.
 *
 *               This class owns:
 *               - Application menus and toolbar
 *               - Log display and progress UI
 *               - Settings tab and persistence
 *
 * @public
 *********************************************************************************************************************************/
class MainWindow final : public QMainWindow
{
        Q_OBJECT

        friend class ProjectSourceDocTests;

    public:
        /*!
         * ****************************************************************************************************************************
         * @brief        Construct the main application window.
         * @param        parent  Optional parent widget.
         ****************************************************************************************************************************/
        explicit MainWindow(QWidget *parent = nullptr);

        /*!
         * ****************************************************************************************************************************
         * @brief        Destructor for MainWindow.
         ****************************************************************************************************************************/
        ~MainWindow() override;

    protected:
        /*!
         * ****************************************************************************************************************************
         * @brief        Handle theme and language changes.
         * @param        event  The change event.
         ****************************************************************************************************************************/
        void changeEvent(QEvent *event) override;

    private slots:
        /*!
         * ****************************************************************************************************************************
         * @brief        Triggered when the user selects "Open Project".
         ****************************************************************************************************************************/
        void onOpenTriggered();

        /*!
         * ****************************************************************************************************************************
         * @brief        Triggered when the user selects "Create Project‑Source.txt".
         ****************************************************************************************************************************/
        void onProjectSourceTriggered();

        /*!
         * ****************************************************************************************************************************
         * @brief        Triggered when the user selects a recent project.
         ****************************************************************************************************************************/
        void onOpenRecentTriggered();

        /*!
         * ****************************************************************************************************************************
         * @brief        Clears the recent project list.
         ****************************************************************************************************************************/
        void onClearRecentTriggered();

        /*!
         * ****************************************************************************************************************************
         * @brief        Browse for a project folder to add to the settings tab.
         ****************************************************************************************************************************/
        void onBrowseProjectFolder();

        /*!
         * ****************************************************************************************************************************
         * @brief        Remove the selected project folder from the settings tab.
         ****************************************************************************************************************************/
        void onRemoveProjectFolder();

        /*!
         * ****************************************************************************************************************************
         * @brief        Browse for an exclude folder pattern.
         ****************************************************************************************************************************/
        void onBrowseExcludeFolder();

        /*!
         * ****************************************************************************************************************************
         * @brief        Remove the selected exclude folder entry.
         ****************************************************************************************************************************/
        void onRemoveExcludeFolder();

    private:
        /* --------------------------- Core UI --------------------------- */

        //! Create all QAction instances.
        void createActions();

        //! Create all application menus.
        void createMenus();

        //! Create the main toolbar.
        void createToolbar();

        //! Create the main tab widget (Log + Settings).
        void createTabs();

        //! Create the status bar and progress UI.
        void createStatusBar();

        //! Begin resetting the log view.
        void beginLogReset();

        //! Append text to the log view.
        void appendToLog(const QString &text);

        //! Display a file inside the log tab with a header.
        void showFileInLogTab(const QString &path, const QString &header);

        //! Return a themed icon based on the current UI theme.
        QIcon themedIcon(const QString &baseName) const;

        //! Open a project folder and optionally show CMakeLists.txt.
        bool openProjectFolder(const QString &folderPath, bool showCMakeIfPresent);

        //! Load recent projects from persistent storage.
        void loadRecentProjects();

        //! Save recent projects to persistent storage.
        void saveRecentProjects() const;

        //! Add a project to the recent list.
        void addRecentProject(const QString &folderPath);

        //! Rebuild the recent projects menu.
        void rebuildRecentProjectsMenu();

        //! Refresh UI elements after theme changes.
        void refreshThemeUi();

        /*!
         * ****************************************************************************************************************************
         * @brief Apply the system (default) theme.
         *
         * Clears any application-level palette or stylesheet overrides and restores the
         * platform’s default style and palette. When selected, the application follows
         * the operating system’s appearance (light/dark) automatically.
         *
         * This function triggers a palette change, which causes theme-aware icons and UI
         * elements to refresh via existing event handlers.
         *****************************************************************************************************************************/
        void applySystemTheme();

        /*!
         * ****************************************************************************************************************************
         * @brief Apply a forced light theme.
         *
         * Applies a light application palette suitable for bright UI environments.
         * This overrides the system theme until changed by the user.
         *
         * Theme-aware icons and widgets are refreshed automatically through palette
         * change events.
         *****************************************************************************************************************************/
        void applyLightTheme();

        /*!
         * ****************************************************************************************************************************
         * @brief Apply a forced dark theme.
         *
         * Applies a dark application palette designed for low-light environments.
         * This overrides the system theme until changed by the user.
         *
         * Theme-aware icons and widgets are refreshed automatically through palette
         * change events.
         *****************************************************************************************************************************/
        void applyDarkTheme();

        /*!
         * ****************************************************************************************************************************
         * @brief Save the selected theme mode to persistent settings.
         * @param mode Selected theme mode.
         *****************************************************************************************************************************/
        void saveThemeMode(ThemeMode mode);

        /*!
         * ********************************************************************************************************************************
         * @brief Restore the persisted theme mode from settings.
         *          * Defaults to system theme if no value is stored.
         * *******************************************************************************************************************************/
        void restoreThemeMode();

        /*!
         * ****************************************************************************************************************************
         * @brief Update the Theme menu checkmarks to match the specified theme mode.
         * @param mode Theme mode that should appear selected in the menu.
         *****************************************************************************************************************************/
        void updateThemeMenuChecks(ThemeMode mode);

        /* --------------------------- Settings Tab --------------------------- */

        //! Create the settings tab UI.
        void createSettingsTab();

        //! Load project folders into the settings tab.
        void loadProjectFoldersForSettingsTab();

        //! Save project folders from the settings tab.
        void saveProjectFoldersFromSettingsTab() const;

        //! Load settings for a specific project.
        void loadSettingsForProject(const QString &projectRoot);

        //! Save settings for a specific project.
        void saveSettingsForProject(const QString &projectRoot) const;

        //! Return the currently selected project in the settings tab.
        QString currentSettingsProject() const;

        //! Enable or disable the settings panel.
        void setSettingsPanelEnabled(bool enabled);

        //! Compute a stable project ID from an absolute path.
        static QString projectIdForPath(const QString &absPath);

        //! Normalize an extension string.
        static QString normalizeExt(const QString &ext);

        //! Parse extension text into a QStringList.
        static QStringList parseExtensionsText(const QString &text);

        //! Default include extensions.
        static QStringList defaultIncludeExts();

        //! Default excluded directories.
        static QStringList defaultExcludeDirs();

        /*!
         * @brief Extract exclude directory patterns from a project's .gitignore file.
         * @param projectRoot Absolute path to the project root.
         * @return List of excluded directory patterns.
         */
        static QStringList excludeDirsFromGitignore(const QString &projectRoot);

    private:
        /* --------------------------- Core Members --------------------------- */
        QTabWidget     *m_tabs{nullptr};
        StatusBarQueue *m_statusQueue{nullptr};
        QTextEdit      *m_logView{nullptr};
        QProgressBar   *m_logProgress{nullptr};
        bool            m_logResetPending{false};

        QString m_openProjectRoot;
        QString m_currentLogFilePath;

        QAction *m_openAction{nullptr};
        QAction *m_projectSourceAction{nullptr};
        QAction *m_exitAction{nullptr};
        QAction *m_usageAction{nullptr};
        QAction *m_aboutAction{nullptr};
        QAction *m_clearRecentAction{nullptr};

        /*!
         * ****************************************************************************************************************************
         * @brief Action to follow the system theme.
         *
         * When selected, clears all application palette overrides and allows the
         * operating system to control the application’s appearance.
         *****************************************************************************************************************************/
        QAction *m_themeSystemAction{nullptr};

        /*!
         * ****************************************************************************************************************************
         * @brief Action to force a light theme.
         *
         * Applies a light palette regardless of system appearance.
         *****************************************************************************************************************************/
        QAction *m_themeLightAction{nullptr};

        /*!
         * ****************************************************************************************************************************
         * @brief Action to force a dark theme.
         *
         * Applies a dark palette regardless of system appearance.
         *****************************************************************************************************************************/
        QAction *m_themeDarkAction{nullptr};

        /*!
         * ****************************************************************************************************************************
         * @brief Exclusive action group for theme selection.
         *
         * Ensures that only one theme option (System, Light, or Dark) can be active
         * at a time in the Tools → Theme menu.
         *****************************************************************************************************************************/
        QActionGroup *m_themeGroup{nullptr};

        QString m_platformStyleName;

        QMenu *m_recentMenu{nullptr};
        QToolBar *m_toolbar{nullptr};

        QStringList m_recentProjects;

        /* --------------------------- Settings Tab Widgets --------------------------- */
        QWidget     *m_settingsTab{nullptr};
        QListWidget *m_projectFoldersList{nullptr};
        QPushButton *m_addProjectFolderBtn{nullptr};
        QPushButton *m_removeProjectFolderBtn{nullptr};

        QLineEdit   *m_includeExtEdit{nullptr};
        QListWidget *m_excludeFoldersList{nullptr};
        QPushButton *m_addExcludeBrowseBtn{nullptr};
        QPushButton *m_removeExcludeBtn{nullptr};

        QLineEdit   *m_backupFolderEdit{nullptr};
        QPushButton *m_backupBrowseBtn{nullptr};

        QString m_settingsSelectedProject;
};

/*!
 * ********************************************************************************************************************************
 * @file         MainWindow.h
 * @brief        End of MainWindow.h.
 *********************************************************************************************************************************/
