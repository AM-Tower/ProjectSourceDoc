/*!
 * ********************************************************************************************************************************
 * @file        MainWindow.h
 * @brief       Declares the main application window for ProjectSource.
 * @details     MainWindow owns and orchestrates the entire Qt Widgets UI for the ProjectSource
 *              application, while delegating all theme handling to MainWindowTheme.cpp.
 *
 *              Responsibilities include:
 *              - File menu (Open project, Open Recent, Exit)
 *              - Tools menu (Create Project‑Source.txt)
 *              - Help menu (Usage, About)
 *              - Central tabs (Log + Settings)
 *
 *              Theme behavior, persistence, icon refresh, and Theme menu creation are implemented
 *              in MainWindowTheme.cpp to allow reuse across applications.
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-04
 *********************************************************************************************************************************/
#pragma once

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QStringList>

/* Forward declarations */
class QAction;
class QActionGroup;
class QTabWidget;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QToolBar;
class StatusBarQueue;
class QCloseEvent;
class QIcon;

/*!
 * ****************************************************************************************************************************
 * @brief Theme mode selection.
 *****************************************************************************************************************************/
enum class ThemeMode
{
    System, /// < Follow the operating system theme
    Light,  /// < Force light application theme
    Dark    /// < Force dark application theme
};

/*!
 * ****************************************************************************************************************************
 * @class   MainWindow
 * @brief   Primary GUI window for the ProjectSource application.
 *
 * @details MainWindow is responsible for all user‑facing interactions and delegates
 *          project source generation and backup logic to ProjectSourceExporter.
 *****************************************************************************************************************************/
class MainWindow final : public QMainWindow
{
        Q_OBJECT
        friend class ProjectSourceDocTests;

    public:
        //! Construct the main application window.
        explicit MainWindow(QWidget *parent = nullptr);
        //! Destroy the main application window.
        ~MainWindow() override;

    protected:
        //! Handle language, palette, and style change events.
        void changeEvent(QEvent *event) override;
        //! Persist window geometry and state on close.
        void closeEvent(QCloseEvent *event) override;

    private slots:
        //! Triggered when the user selects File → Open.
        void onOpenTriggered();
        //! Triggered when the user selects Tools → Create Project‑Source.txt.
        void onProjectSourceTriggered();
        //! Triggered when the user selects a recent project.
        void onOpenRecentTriggered();
        //! Clear the recent project list.
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
        /* --------------------------- Theme (implemented in MainWindowTheme.cpp) --------------------------- */

               //! Create the Tools → Theme submenu.
        void createThemeMenu(QMenu *toolsMenu);
        //! Apply a theme mode and synchronize UI state.
        void applyThemeMode(ThemeMode mode, bool persist);
        //! Restore the persisted theme mode from settings.
        void restoreThemeMode();
        //! Load the persisted theme mode from settings.
        ThemeMode loadThemeMode() const;
        //! Save the selected theme mode to settings.
        void saveThemeMode(ThemeMode mode);
        //! Update Theme menu checkmarks.
        void updateThemeMenuChecks(ThemeMode mode);
        //! Retranslate Theme menu text.
        void retranslateThemeMenu();
        //! Handle palette/style/theme change events.
        void handleThemeChangeEvent(QEvent *event);
        //! Refresh all theme‑aware UI elements.
        void refreshThemeUi();
        //! Apply the system (default) theme.
        void applySystemTheme();
        //! Apply a forced light theme.
        void applyLightTheme();
        //! Apply a forced dark theme.
        void applyDarkTheme();
        //! Refresh a QAction icon if it declares an "iconName" property.
        void refreshActionIcon(QAction *action);
        //! Return a theme‑aware icon.
        QIcon themedIcon(const QString &baseName) const;
        //! Load and validate a project tree (informational only).
        void loadProjectTree(const QString &projectRoot);

        /* --------------------------- Core UI --------------------------- */

               //! Create all QAction instances.
        void createActions();
        //! Create the menu bar.
        void createMenus();
        //! Create the main toolbar.
        void createToolbar();
        //! Create the central tab widget (Log + Settings).
        void createTabs();
        //! Create the status bar.
        void createStatusBar();
        //! Restore persisted window geometry and state.
        void restoreWindowState();
        //! Persist window geometry and state.
        void saveWindowState() const;

        /* --------------------------- Log --------------------------- */

               //! Begin resetting the log view.
        void beginLogReset();
        //! Append text to the log view.
        void appendToLog(const QString &text);
        //! Display a file in the log tab.
        void showFileInLogTab(const QString &path, const QString &header);

        /* --------------------------- Projects --------------------------- */

               //! Open a project folder and optionally show CMakeLists.txt.
        bool openProjectFolder(const QString &folderPath, bool showCMakeIfPresent);
        //! Load recent projects from settings.
        void loadRecentProjects();
        //! Save recent projects to settings.
        void saveRecentProjects() const;
        //! Add a project to the recent list.
        void addRecentProject(const QString &folderPath);
        //! Rebuild the recent projects menu.
        void rebuildRecentProjectsMenu();
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

               //! Parse extension text into a normalized QStringList.
        static QStringList parseExtensionsText(const QString &text);
        //! Default include extensions.
        static QStringList defaultIncludeExts();
        //! Default excluded directories.
        static QStringList defaultExcludeDirs();
        //! Extract exclude directories from .gitignore.
        static QStringList excludeDirsFromGitignore(const QString &projectRoot);

    private:
        /* --------------------------- Core Members --------------------------- */

        QTabWidget *m_tabs{nullptr};                   /// < Main tab widget
        StatusBarQueue *m_statusQueue{nullptr};        /// < Status bar message queue
        QPlainTextEdit *m_logView{nullptr};             /// < Log output view
        QProgressBar *m_logProgress{nullptr};           /// < Log progress indicator
        bool m_logResetPending{false};                  /// < Pending log reset flag

        QString m_openProjectRoot;                      /// < Last opened project root
        QString m_currentLogFilePath;                   /// < Currently displayed log file
        QString m_activeProjectRoot;                    /// < Active project root

        QAction *m_openAction{nullptr};                 /// < Open project action
        QAction *m_projectSourceAction{nullptr};        /// < Create Project‑Source.txt action
        QAction *m_exitAction{nullptr};                 /// < Exit application action
        QAction *m_usageAction{nullptr};                /// < Usage/help action
        QAction *m_aboutAction{nullptr};                /// < About dialog action
        QAction *m_clearRecentAction{nullptr};          /// < Clear recent projects action

        QMenu *m_themeMenu{nullptr};                    /// < Tools → Theme submenu
        QAction *m_themeSystemAction{nullptr};          /// < System theme action
        QAction *m_themeLightAction{nullptr};           /// < Light theme action
        QAction *m_themeDarkAction{nullptr};            /// < Dark theme action
        QActionGroup *m_themeGroup{nullptr};            /// < Exclusive theme action group
        QString m_platformStyleName;                    /// < Platform default style name

        QMenu *m_recentMenu{nullptr};                   /// < Recent projects menu
        QToolBar *m_toolbar{nullptr};                   /// < Main toolbar
        QStringList m_recentProjects;                   /// < Recent project list

        /* --------------------------- Settings Widgets --------------------------- */

        QWidget *m_settingsTab{nullptr};                /// < Settings tab widget
        QListWidget *m_projectFoldersList{nullptr};     /// < Project folders list
        QPushButton *m_addProjectFolderBtn{nullptr};    /// < Add project folder button
        QPushButton *m_removeProjectFolderBtn{nullptr}; /// < Remove project folder button
        QLineEdit *m_includeExtEdit{nullptr};            /// < Include extensions input
        QListWidget *m_excludeFoldersList{nullptr};     /// < Excluded folders list
        QPushButton *m_addExcludeBrowseBtn{nullptr};    /// < Browse exclude folder button
        QPushButton *m_removeExcludeBtn{nullptr};       /// < Remove exclude folder button
        QLineEdit *m_backupFolderEdit{nullptr};          /// < Backup folder input
        QPushButton *m_backupBrowseBtn{nullptr};        /// < Browse backup folder button

        QLabel *m_cursorPosLabel{nullptr};              /// < Cursor position label
};

/*!
 * ********************************************************************************************************************************
 * @brief End of MainWindow.h
 *********************************************************************************************************************************/