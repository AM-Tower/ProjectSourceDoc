/*!
 * ********************************************************************************************************************************
 * @file        MainWindow.h
 * @brief       Declares the main application window for ProjectSource.
 * @details     MainWindow owns and orchestrates the entire Qt Widgets UI for the ProjectSource
 *              application.
 *
 *              Responsibilities include:
 *              - File menu (Open project, Open Recent, Exit)
 *              - Tools menu (Create Project‑Source.txt)
 *              - Help menu (Usage, About)
 *              - Central tabs (Log + Settings)
 *
 *              The Settings tab supports:
 *              - Editable project folder list (per‑project selection)
 *              - Include file extensions (comma/space/semicolon separated)
 *              - Exclude folder patterns with browse support
 *              - Backup base folder selection with timestamped backups
 *
 *              All per‑project settings are persisted using QSettings and applied when generating
 *              Project‑Source.txt and performing project backups.
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-04
 *********************************************************************************************************************************/
#pragma once

#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QStringList>

/* Forward declarations */
class QAction;
class QListWidget;
class QMenu;
class QProgressBar;
class QTabWidget;
class QToolBar;
class StatusBarQueue;
class QActionGroup;
class QCloseEvent;

static constexpr const char *kSettingsThemeMode = "ui/themeMode"; ///< Settings key for persisted theme mode

/*!
 * ********************************************************************************************************************************
 * @brief Theme Mode
 *********************************************************************************************************************************/
enum class ThemeMode
{
    System, ///< Follow the operating system theme
    Light,  ///< Force light application theme
    Dark    ///< Force dark application theme
};

/*!
 * ********************************************************************************************************************************
 * @class MainWindow
 * @brief Primary GUI window for the ProjectSource application.
 *
 * @details MainWindow is responsible for all user‑facing interactions and delegates project
 *          source generation and backup logic to ProjectSourceExporter.
 *
 * @public
 *********************************************************************************************************************************/
class MainWindow final : public QMainWindow
{
        Q_OBJECT
        friend class ProjectSourceDocTests;

    public:
        //! Construct the main application window.
        explicit MainWindow(QWidget *parent = nullptr);

        //! Destructor for MainWindow.
        ~MainWindow() override;

    protected:
        //! Handle theme and language changes.
        void changeEvent(QEvent *event) override;

        //! Persist window geometry and state on close.
        void closeEvent(QCloseEvent *event) override;

    private slots:
        //! Triggered when the user selects "Open Project".
        void onOpenTriggered();

        //! Triggered when the user selects "Create Project‑Source.txt".
        void onProjectSourceTriggered();

        //! Triggered when the user selects a recent project.
        void onOpenRecentTriggered();

        //! Clears the recent project list.
        void onClearRecentTriggered();

        //! Browse for a project folder to add to the settings tab.
        void onBrowseProjectFolder();

        //! Remove the selected project folder from the settings tab.
        void onRemoveProjectFolder();

        //! Browse for an exclude folder pattern.
        void onBrowseExcludeFolder();

        //! Remove the selected exclude folder entry.
        void onRemoveExcludeFolder();

        //! Show the About dialog.
        void showAboutDialog();

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

        //! Apply the system (default) theme.
        void applySystemTheme();

        //! Apply a forced light theme.
        void applyLightTheme();

        //! Apply a forced dark theme.
        void applyDarkTheme();

        //! Save the selected theme mode to persistent settings.
        void saveThemeMode(ThemeMode mode);

        //! Restore the persisted theme mode from settings.
        void restoreThemeMode();

        //! Update the Theme menu checkmarks to match the specified theme mode.
        void updateThemeMenuChecks(ThemeMode mode);

        //! Restore persisted window geometry and state.
        void restoreWindowState();

        //! Persist window geometry and state.
        void saveWindowState() const;

        //! Loads and analyzes a project folder at runtime.
        void loadProjectTree(const QString &projectRoot);

        //! Set the currently active project.
        void setActiveProject(const QString &projectRoot, bool updateSettingsSelection);

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

        //! Normalize an extension string.
        static QString normalizeExt(const QString &ext);

        //! Parse extension text into a QStringList.
        static QStringList parseExtensionsText(const QString &text);

        //! Default include extensions.
        static QStringList defaultIncludeExts();

        //! Default excluded directories.
        static QStringList defaultExcludeDirs();

        //! Extract exclude directory patterns from a project's .gitignore file.
        static QStringList excludeDirsFromGitignore(const QString &projectRoot);

    private:
        /* --------------------------- Core Members --------------------------- */

        QTabWidget *m_tabs{nullptr};            ///< Main tab widget containing Log and Settings tabs
        StatusBarQueue *m_statusQueue{nullptr}; ///< Queued status bar message handler
        QPlainTextEdit *m_logView{nullptr};     ///< Log output text view
        QProgressBar *m_logProgress{nullptr};   ///< Progress indicator for log operations
        bool m_logResetPending{false};          ///< Indicates a pending log reset
        QString m_openProjectRoot;              ///< Root path of the opened project
        QString m_currentLogFilePath;           ///< Currently displayed log file path
        QString m_activeProjectRoot;            ///< Currently active project root path

        QAction *m_openAction{nullptr};          ///< Open project action
        QAction *m_projectSourceAction{nullptr}; ///< Create Project‑Source.txt action
        QAction *m_exitAction{nullptr};          ///< Exit application action
        QAction *m_usageAction{nullptr};         ///< Usage/help action
        QAction *m_aboutAction{nullptr};         ///< About dialog action
        QAction *m_clearRecentAction{nullptr};   ///< Clear recent projects action

        QAction *m_themeSystemAction{nullptr}; ///< System theme selection action
        QAction *m_themeLightAction{nullptr};  ///< Light theme selection action
        QAction *m_themeDarkAction{nullptr};   ///< Dark theme selection action
        QActionGroup *m_themeGroup{nullptr};   ///< Exclusive theme action group

        QString m_platformStyleName;  ///< Name of the platform style in use
        QMenu *m_recentMenu{nullptr}; ///< Recent projects menu
        QToolBar *m_toolbar{nullptr}; ///< Main application toolbar
        QStringList m_recentProjects; ///< List of recently opened projects

        /* --------------------------- Settings Tab Widgets --------------------------- */

        QWidget *m_settingsTab{nullptr};                ///< Settings tab container widget
        QListWidget *m_projectFoldersList{nullptr};     ///< List of project folders
        QPushButton *m_addProjectFolderBtn{nullptr};    ///< Add project folder button
        QPushButton *m_removeProjectFolderBtn{nullptr}; ///< Remove project folder button
        QLineEdit *m_includeExtEdit{nullptr};           ///< Include extensions input
        QListWidget *m_excludeFoldersList{nullptr};     ///< Excluded folders list
        QPushButton *m_addExcludeBrowseBtn{nullptr};    ///< Browse exclude folder button
        QPushButton *m_removeExcludeBtn{nullptr};       ///< Remove exclude folder button
        QLineEdit *m_backupFolderEdit{nullptr};         ///< Backup base folder input
        QPushButton *m_backupBrowseBtn{nullptr};        ///< Browse backup folder button
        QString m_settingsSelectedProject;              ///< Currently selected settings project
        QLabel *m_cursorPosLabel{nullptr};              ///< Cursor position status label
};

/*!
 * ********************************************************************************************************************************
 * @brief End of MainWindow.h
 *********************************************************************************************************************************/
