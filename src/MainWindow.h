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
 *               Project‑Source.txt and performing project backups.
 *
 * @author       Jeffrey Scott Flesher
 * @date         2026-03-26
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

/*!
 * ********************************************************************************************************************************
 * @class        MainWindow
 * @brief        Primary GUI window for the ProjectSource application.
 *
 * @details      MainWindow is responsible for all user‑facing interactions and delegates
 *               project source generation and backup logic to ProjectSourceExporter.
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

    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        void changeEvent(QEvent *event) override;

    private slots:
        void onOpenTriggered();
        void onProjectSourceTriggered();
        void onOpenRecentTriggered();
        void onClearRecentTriggered();

        void onBrowseProjectFolder();
        void onRemoveProjectFolder();
        void onBrowseExcludeFolder();
        void onRemoveExcludeFolder();

    private:
        /* --------------------------- Core UI --------------------------- */
        void createActions();
        void createMenus();
        void createToolbar();
        void createTabs();
        void createStatusBar();

        void beginLogReset();
        void appendToLog(const QString &text);
        void showFileInLogTab(const QString &path, const QString &header);

        QIcon themedIcon(const QString &baseName) const;
        bool openProjectFolder(const QString &folderPath, bool showCMakeIfPresent);

        void loadRecentProjects();
        void saveRecentProjects() const;
        void addRecentProject(const QString &folderPath);
        void rebuildRecentProjectsMenu();
        void refreshThemeUi();

        /* --------------------------- Settings Tab --------------------------- */
        void createSettingsTab();
        void loadProjectFoldersForSettingsTab();
        void saveProjectFoldersFromSettingsTab() const;
        void loadSettingsForProject(const QString &projectRoot);
        void saveSettingsForProject(const QString &projectRoot) const;

        QString currentSettingsProject() const;
        void setSettingsPanelEnabled(bool enabled);

        static QString projectIdForPath(const QString &absPath);
        static QString normalizeExt(const QString &ext);
        static QStringList parseExtensionsText(const QString &text);
        static QStringList defaultIncludeExts();
        static QStringList defaultExcludeDirs();

        /*!
         * ********************************************************************************************************************************
         * @brief        Get exclude list from .gitignore.
         *********************************************************************************************************************************/
        QStringList excludeDirsFromGitignore(const QString &projectRoot);

    private:
        /* --------------------------- Core Members --------------------------- */
        QTabWidget     *m_tabs{nullptr};
        StatusBarQueue *m_statusQueue{nullptr};
        QTextEdit      *m_logView{nullptr};
        QProgressBar   *m_logProgress{nullptr};
        bool m_logResetPending{false};

        QString m_openProjectRoot;
        QString m_currentLogFilePath;

        QAction  *m_openAction{nullptr};
        QAction  *m_projectSourceAction{nullptr};
        QAction  *m_exitAction{nullptr};
        QAction  *m_usageAction{nullptr};
        QAction  *m_aboutAction{nullptr};
        QAction  *m_clearRecentAction{nullptr};
        QMenu    *m_recentMenu{nullptr};
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
