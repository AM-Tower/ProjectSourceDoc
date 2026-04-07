/*!
 * ********************************************************************************************************************************
 * @file         MainWindow.cpp
 * @brief        Implements the main GUI window for ProjectSource.
 * @details      Provides menus, toolbar, MRU recent projects, a Log tab, and a Settings tab with
 *               per‑project include/exclude rules and backup configuration.
 *
 *               MainWindow orchestrates user interaction only and delegates all generation and
 *               backup logic to ProjectSourceExporter.
 *
 * @author       Jeffrey Scott Flesher with AI: Copilot
 * @date         2026-04-04
 *********************************************************************************************************************************/

#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QProgressBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QStyle>
#include <QStandardItemModel>
#include <LineNumberPlainTextEdit.h>
#include <QGuiApplication>
#include <QScreen>
#include <QLineEdit>

#include "AboutDialog.h"
#include "AppLogUtils.h"
#include "EmbeddedDocs.h"
#include "ProjectSourceExporter.h"
#include "StatusBarQueue.h"
#include "ProjectPaths.h"

namespace
{
    /* ------------------------------------------------------------------------------------------------
     * Persistent settings keys
     * ------------------------------------------------------------------------------------------------ */
    static constexpr const char *kSettingsLastFolder = "ui/lastProjectFolder";
    static constexpr const char *kSettingsRecentList = "ui/recentProjectFolders";
    static constexpr const char *kSettingsProjectFolders = "ui/projectFolders";
    static constexpr int kMaxRecentProjects = 10;
}

/*!
 * ****************************************************************************************************************************
 * @brief Constructs the main application window.
 *****************************************************************************************************************************/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("ProjectSource"));
    resize(1100, 700);
    psd::log::clearLogFilesAtStartup();
    // Capture the platform default style name before we ever force Fusion.
    m_platformStyleName = qApp->style()->objectName();
    loadRecentProjects();
    createActions();
    createMenus();
    createToolbar();
    createTabs();
    createStatusBar();
    restoreThemeMode();
    restoreWindowState();
}

/*!
 * ********************************************************************************************************************************
 * @brief        Destroys the main application window.
 *********************************************************************************************************************************/
MainWindow::~MainWindow() = default;

/*!
 * ****************************************************************************************************************************
 * @brief   Restore persisted window geometry and state.
 *****************************************************************************************************************************/
void MainWindow::restoreWindowState()
{
    QSettings settings;

    const QByteArray geometry = settings.value(QStringLiteral("ui/windowGeometry")).toByteArray();
    const QByteArray state = settings.value(QStringLiteral("ui/windowState")).toByteArray();
    const bool wasMaximized = settings.value(QStringLiteral("ui/windowMaximized"), false).toBool();

    if (!geometry.isEmpty()) restoreGeometry(geometry);
    if (!state.isEmpty()) restoreState(state);

    // Ensure the restored window is visible on at least one screen
    QRect frame = frameGeometry();
    bool onScreen = false;

    for (QScreen *screen : QGuiApplication::screens())
    {
        if (screen->availableGeometry().intersects(frame))
        {
            onScreen = true;
            break;
        }
    }

    if (!onScreen)
    {
        // Fallback: center on primary screen with a reasonable size
        QScreen *primary = QGuiApplication::primaryScreen();
        if (primary)
        {
            const QRect avail = primary->availableGeometry();
            resize(avail.size() * 0.75);
            move(avail.center() - rect().center());
        }
    }

    if (wasMaximized) showMaximized();
}

/*!
 * ****************************************************************************************************************************
 * @brief   Persist window geometry and state.
 *****************************************************************************************************************************/
void MainWindow::saveWindowState() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("ui/windowMaximized"), isMaximized());
    // Only save normal geometry when not maximized
    if (!isMaximized()) { settings.setValue(QStringLiteral("ui/windowGeometry"), saveGeometry()); }
    settings.setValue(QStringLiteral("ui/windowState"), saveState());
}

/*!
 * ****************************************************************************************************************************
 * @brief   Persist window state before closing.
 * @param   event Close event.
 *****************************************************************************************************************************/
void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();
    QMainWindow::closeEvent(event);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Loads MRU list and last project folder from QSettings.
 *********************************************************************************************************************************/
void MainWindow::loadRecentProjects()
{
    QSettings settings;

    m_recentProjects = settings.value(QString::fromLatin1(kSettingsRecentList)).toStringList();

    m_openProjectRoot = settings.value(QString::fromLatin1(kSettingsLastFolder)).toString();

    QStringList cleaned;
    cleaned.reserve(m_recentProjects.size());

    for (const QString &p : std::as_const(m_recentProjects))
    {
        const QString abs = QDir(p).absolutePath();
        if (!abs.trimmed().isEmpty() && QDir(abs).exists() && !cleaned.contains(abs)) cleaned.append(abs);
    }

    m_recentProjects = cleaned;
}

/*!
 * ********************************************************************************************************************************
 * @brief        Saves MRU list and last project folder to QSettings.
 *********************************************************************************************************************************/
void MainWindow::saveRecentProjects() const
{
    QSettings settings;
    settings.setValue(QString::fromLatin1(kSettingsRecentList), m_recentProjects);
    settings.setValue(QString::fromLatin1(kSettingsLastFolder), m_openProjectRoot);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Adds a project folder to the MRU list.
 *********************************************************************************************************************************/
void MainWindow::addRecentProject(const QString &folderPath)
{
    const QString abs = QDir(folderPath).absolutePath();
    if (abs.trimmed().isEmpty()) return;

    m_recentProjects.removeAll(abs);
    m_recentProjects.prepend(abs);

    while (m_recentProjects.size() > kMaxRecentProjects) m_recentProjects.removeLast();

    saveRecentProjects();
    rebuildRecentProjectsMenu();

    QSettings s;
    QStringList list = s.value(QString::fromLatin1(kSettingsProjectFolders)).toStringList();

    if (!list.contains(abs))
    {
        list.prepend(abs);
        s.setValue(QString::fromLatin1(kSettingsProjectFolders), list);

        if (m_projectFoldersList) loadProjectFoldersForSettingsTab();
    }
}

/*!
 * ****************************************************************************************************************************
 * @brief Rebuilds the File → Open Recent menu.
 *****************************************************************************************************************************/
void MainWindow::rebuildRecentProjectsMenu()
{
    if (!m_recentMenu) return;

    m_recentMenu->clear();

    if (m_recentProjects.isEmpty())
    {
        QAction *empty = m_recentMenu->addAction(tr("(No recent projects)"));
        empty->setEnabled(false);
        return;
    }

    for (const QString &path : std::as_const(m_recentProjects))
    {
        QAction *a = m_recentMenu->addAction(path);
        a->setProperty("iconName", QStringLiteral("doc"));
        a->setIcon(themedIcon(QStringLiteral("doc")));
        a->setData(path);
        connect(a, &QAction::triggered, this, &MainWindow::onOpenRecentTriggered);
    }

    m_recentMenu->addSeparator();

    if (!m_clearRecentAction)
    {
        m_clearRecentAction = new QAction(tr("Clear Recent"), this);
        connect(m_clearRecentAction, &QAction::triggered, this, &MainWindow::onClearRecentTriggered);
    }

    m_recentMenu->addAction(m_clearRecentAction);
}

/*!
 * ****************************************************************************************************************************
 * @brief Creates application actions.
 *****************************************************************************************************************************/
void MainWindow::createActions()
{
    m_openAction = new QAction(themedIcon(QStringLiteral("doc")), tr("&Open…"), this);
    m_openAction->setProperty("iconName", QStringLiteral("doc"));
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenTriggered);

    m_projectSourceAction = new QAction(themedIcon(QStringLiteral("update")), tr("Create Project‑Source.txt"), this);
    m_projectSourceAction->setProperty("iconName", QStringLiteral("update"));
    connect(m_projectSourceAction, &QAction::triggered, this, &MainWindow::onProjectSourceTriggered);

    m_exitAction = new QAction(themedIcon(QStringLiteral("exit")), tr("E&xit"), this);
    m_exitAction->setProperty("iconName", QStringLiteral("exit"));
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    m_usageAction = new QAction(themedIcon(QStringLiteral("help")), tr("Usage"), this);
    m_usageAction->setProperty("iconName", QStringLiteral("help"));
    connect(m_usageAction, &QAction::triggered, this, [this]() { psd::docs::showUsageDialog(this); });

    m_aboutAction = new QAction(themedIcon(QStringLiteral("help")), tr("About"), this);
    m_aboutAction->setProperty("iconName", QStringLiteral("help"));
    connect(m_aboutAction, &QAction::triggered, this, [this]() { AboutDialog dlg(this); dlg.exec(); });
}

/*!
 * ****************************************************************************************************************************
 * @brief Creates the menu bar.
 *****************************************************************************************************************************/
void MainWindow::createMenus()
{
    // File
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);

    m_recentMenu = fileMenu->addMenu(tr("Open &Recent"));
    rebuildRecentProjectsMenu();

    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    // Tools
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(m_projectSourceAction);

    // ✅ Theme injected here — NOT created here
    createThemeMenu(toolsMenu);

    // Help
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_usageAction);
    helpMenu->addAction(m_aboutAction);
}

/*!
 * ****************************************************************************************************************************
 * @brief Creates the main toolbar.
 *****************************************************************************************************************************/
void MainWindow::createToolbar()
{
    m_toolbar = addToolBar(tr("Main Toolbar"));
    m_toolbar->setMovable(false);

    m_toolbar->addAction(m_openAction);

    auto *recentBtn = new QToolButton(m_toolbar);
    recentBtn->setText(tr("Recent"));
    recentBtn->setProperty("iconName", QStringLiteral("doc"));
    recentBtn->setIcon(themedIcon(QStringLiteral("doc")));
    recentBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    recentBtn->setPopupMode(QToolButton::InstantPopup);
    recentBtn->setMenu(m_recentMenu);
    m_toolbar->addWidget(recentBtn);

    m_toolbar->addSeparator();
    m_toolbar->addAction(m_projectSourceAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_exitAction);
}

/*!
 * ****************************************************************************************************************************
 * @brief Creates the central tab widget (Log + Settings).
 *****************************************************************************************************************************/
void MainWindow::createTabs()
{
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    QWidget *logTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(logTab);

    m_logProgress = new QProgressBar(logTab);
    m_logProgress->setRange(0, 100);
    layout->addWidget(m_logProgress);

    m_logView = new LineNumberPlainTextEdit(logTab);
    m_logView->setReadOnly(true);
    m_logView->setWordWrapMode(QTextOption::NoWrap);
    m_logView->setUndoRedoEnabled(false);
    layout->addWidget(m_logView, 1);

    m_tabs->addTab(logTab, themedIcon(QStringLiteral("log")), tr("Log"));

    createSettingsTab();

    m_tabs->setProperty("tabIconNames", QStringList{QStringLiteral("log"), QStringLiteral("settings")});

    connect(m_logView, &QPlainTextEdit::cursorPositionChanged, this,
            [this]()
            {
                if (!m_cursorPosLabel) return;
                QTextCursor cursor = m_logView->textCursor();
                const int line = cursor.blockNumber() + 1;
                const int column = cursor.positionInBlock() + 1;
                m_cursorPosLabel->setText(tr("Ln %1, Col %2").arg(line).arg(column));
            });
}

/*!
 * ********************************************************************************************************************************
 * @brief        Creates the status bar.
 *********************************************************************************************************************************/
void MainWindow::createStatusBar()
{
    m_statusQueue = new StatusBarQueue(statusBar(), this);
    m_statusQueue->enqueue(tr("Ready"), 2);

    m_cursorPosLabel = new QLabel(tr("Ln 1, Col 1"), this);

    // Ensure stable width so the label never touches the right edge
    // and does not resize as line/column numbers grow.
    const QFontMetrics fm(m_cursorPosLabel->font());
    const int padding = fm.horizontalAdvance(QLatin1Char(' ')) * 2;
    const int minWidth = fm.horizontalAdvance(QStringLiteral("Ln 99999, Col 999")) + padding;

    m_cursorPosLabel->setMinimumWidth(minWidth);

    statusBar()->addPermanentWidget(m_cursorPosLabel);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Marks the log for reset on next append.
 *********************************************************************************************************************************/
void MainWindow::beginLogReset()
{
    m_logResetPending = true;
}

/*!
 * ********************************************************************************************************************************
 * @brief        Appends text to the Log tab.
 *********************************************************************************************************************************/
void MainWindow::appendToLog(const QString &text)
{
    if (!m_logView) return;

    if (m_logResetPending)
    {
        m_logView->clear();
        m_logResetPending = false;
    }

    psd::log::logAppend(m_logView, text);
    m_logView->moveCursor(QTextCursor::End);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Displays a file in the Log tab.
 *********************************************************************************************************************************/
void MainWindow::showFileInLogTab(const QString &path, const QString &header)
{
    beginLogReset();

    QString text = header + QLatin1String("\n\n");

    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        text += QString::fromUtf8(file.readAll());
    else
        text += tr("ERROR: Unable to open file.");

    appendToLog(text);
    m_currentLogFilePath = path;

    if (m_tabs) m_tabs->setCurrentIndex(0);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Opens a project folder and updates MRU.
 *********************************************************************************************************************************/
bool MainWindow::openProjectFolder(const QString &folderPath, bool showCMakeIfPresent)
{
    const QString abs = QDir(folderPath).absolutePath();

    if (abs.trimmed().isEmpty() || !QDir(abs).exists()) return false;

    setActiveProject(abs, /*updateSettingsSelection=*/true);
    addRecentProject(abs);

    if (m_statusQueue) m_statusQueue->enqueue(tr("Opened project: %1").arg(abs), 3);

    if (showCMakeIfPresent)
    {
        const QString cmakePath = QDir(abs).filePath(QStringLiteral("CMakeLists.txt"));

        if (QFileInfo::exists(cmakePath))
        {
            showFileInLogTab(cmakePath, tr("Opened project: %1").arg(abs));
            return true;
        }
    }

    appendToLog(tr("Opened project root: %1\n").arg(abs));
    return true;
}

/*!
 * ****************************************************************************************************************************
 * @brief   Handles File → Open.
 * @details Opens a project folder and synchronizes the Settings tab selection.
 *****************************************************************************************************************************/
void MainWindow::onOpenTriggered()
{
    const QString start = !m_openProjectRoot.isEmpty() ? m_openProjectRoot : QDir::homePath();

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Project Folder"), start, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) return;

    if (!openProjectFolder(dir, true)) return;

    /* ------------------------------------------------------------------------------------------------
     * Match Recent-project behavior:
     * - Ensure Settings tab is active
     * - Ensure selected project is visible
     * ------------------------------------------------------------------------------------------------ */
    if (m_tabs) m_tabs->setCurrentIndex(1);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Handles File → Open Recent.
 *********************************************************************************************************************************/
void MainWindow::onOpenRecentTriggered()
{
    QAction *a = qobject_cast<QAction *>(sender());
    if (!a) return;

    const QString path = a->data().toString();

    if (!openProjectFolder(path, true))
    {
        m_recentProjects.removeAll(path);
        saveRecentProjects();
        rebuildRecentProjectsMenu();

        if (m_statusQueue) m_statusQueue->enqueue(tr("Recent project not found; removed from list."), 3);
    }
}

/*!
 * ********************************************************************************************************************************
 * @brief        Clears the MRU list.
 *********************************************************************************************************************************/
void MainWindow::onClearRecentTriggered()
{
    m_recentProjects.clear();
    saveRecentProjects();
    rebuildRecentProjectsMenu();
    if (m_statusQueue) m_statusQueue->enqueue(tr("Recent projects cleared."), 2);
}

/*!
 * ****************************************************************************************************************************
 * @brief Extract exclude directory patterns from a project's .gitignore file.
 * @details Directory rules only:
 *          - ignores comments/blank lines
 *          - ignores negations (!)
 *          - accepts only lines ending with '/'
 *          - strips trailing '/' and normalizes "dir/ *" (Space added before * because it breaks comment)
 * @param[in] projectRoot Absolute project root.
 * @return Directory exclusion patterns (no trailing slash).
 *****************************************************************************************************************************/
QStringList MainWindow::excludeDirsFromGitignore(const QString &projectRoot)
{
    QStringList result;
    const QString gitignorePath = QDir(projectRoot).filePath(QStringLiteral(".gitignore"));
    QFile file(gitignorePath);

    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) return result;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        if (line.startsWith('!')) continue;

        if (!line.endsWith('/')) continue;
        line.chop(1);

        if (line.endsWith(QStringLiteral("/*"))) line.chop(2);

        if (!line.isEmpty() && !result.contains(line)) result.append(line);
    }

    return result;
}

/*!
 * ********************************************************************************************************************************
 * @brief   setActiveProject
 *********************************************************************************************************************************/
void MainWindow::setActiveProject(const QString &projectRoot, bool updateSettingsSelection)
{
    const QString abs = QDir(projectRoot).absolutePath();
    if (abs.isEmpty() || !QDir(abs).exists()) return;

    m_activeProjectRoot = abs;
    m_openProjectRoot = abs;

    if (updateSettingsSelection && m_projectFoldersList)
    {
        for (int i = 0; i < m_projectFoldersList->count(); ++i)
        {
            QListWidgetItem *item = m_projectFoldersList->item(i);
            if (QDir(item->text()).absolutePath() == abs)
            {
                QSignalBlocker b(m_projectFoldersList);
                m_projectFoldersList->setCurrentItem(item);

                // ✅ IMPORTANT: initialize settings panel explicitly
                setSettingsPanelEnabled(true);
                loadSettingsForProject(abs);
                break;
            }
        }
    }
}

/*!
 * ********************************************************************************************************************************
 * @brief        Returns default file extensions to include.
 *********************************************************************************************************************************/
QStringList MainWindow::defaultIncludeExts()
{
    return {QStringLiteral("cpp"), QStringLiteral("h"), QStringLiteral("md")};
}

/*!
 * ****************************************************************************************************************************
 * @brief Returns default excluded directory patterns for backward‑compatible calls.
 *****************************************************************************************************************************/
QStringList MainWindow::defaultExcludeDirs()
{
    return {
        QStringLiteral("build*"),
        QStringLiteral("build_release"),
        QStringLiteral("build-*"),
        QStringLiteral("cmake-build-*"),
        QStringLiteral("deploy"),
        QStringLiteral("deploy-*"),
        QStringLiteral("AppDir"),
        QStringLiteral(".rcc"),
        QStringLiteral("CMakeFiles"),
        QStringLiteral(".git"),
        QStringLiteral(".qtcreator"),
        QStringLiteral("engines"),
        QStringLiteral("0-Archive")
    };
}

/*!
 * ********************************************************************************************************************************
 * @brief        Loads project folder list into the Settings tab.
 *********************************************************************************************************************************/
void MainWindow::loadProjectFoldersForSettingsTab()
{
    if (!m_projectFoldersList) return;
    QSettings s;
    QStringList list = s.value(QString::fromLatin1(kSettingsProjectFolders)).toStringList();
    if (list.isEmpty()) list = m_recentProjects;
    m_projectFoldersList->blockSignals(true);
    m_projectFoldersList->clear();

    for (const QString &p : std::as_const(list))
    {
        const QString abs = QDir(p).absolutePath();
        if (abs.trimmed().isEmpty()) continue;

        auto *item = new QListWidgetItem(abs, m_projectFoldersList);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    m_projectFoldersList->blockSignals(false);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Creates and initializes the Settings tab UI.
 *********************************************************************************************************************************/
void MainWindow::createSettingsTab()
{
    m_settingsTab = new QWidget(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, m_settingsTab);
    splitter->setChildrenCollapsible(false);

    /* ===================== Left: Project folders ===================== */
    QWidget *left = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(left);

    leftLayout->addWidget(new QLabel(tr("Project folders"), left));

    m_projectFoldersList = new QListWidget(left);
    m_projectFoldersList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectFoldersList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    leftLayout->addWidget(m_projectFoldersList, 1);

    QHBoxLayout *projBtns = new QHBoxLayout();
    m_addProjectFolderBtn = new QPushButton(tr("Browse…"), left);
    m_removeProjectFolderBtn = new QPushButton(tr("Remove"), left);

    projBtns->addWidget(m_addProjectFolderBtn);
    projBtns->addWidget(m_removeProjectFolderBtn);
    leftLayout->addLayout(projBtns);

    /* ===================== Right: Settings ===================== */
    QWidget *right = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(right);

    /* ---------- Include extensions ---------- */
    QGroupBox *includeBox = new QGroupBox(tr("Include file extensions"), right);
    QVBoxLayout *includeLayout = new QVBoxLayout(includeBox);

    includeLayout->addWidget(new QLabel(tr("Comma / space / semicolon separated (e.g. cpp, h, md)"), includeBox));

    m_includeExtEdit = new QLineEdit(includeBox);
    includeLayout->addWidget(m_includeExtEdit);

    rightLayout->addWidget(includeBox);

    /* ---------- Exclude folders ---------- */
    QGroupBox *excludeBox = new QGroupBox(tr("Exclude folders"), right);
    QVBoxLayout *excludeLayout = new QVBoxLayout(excludeBox);

    m_excludeFoldersList = new QListWidget(excludeBox);
    m_excludeFoldersList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    excludeLayout->addWidget(m_excludeFoldersList, 1);

    QHBoxLayout *exBtns = new QHBoxLayout();
    m_addExcludeBrowseBtn = new QPushButton(tr("Browse…"), excludeBox);
    m_removeExcludeBtn = new QPushButton(tr("Remove"), excludeBox);

    exBtns->addWidget(m_addExcludeBrowseBtn);
    exBtns->addWidget(m_removeExcludeBtn);
    excludeLayout->addLayout(exBtns);

    rightLayout->addWidget(excludeBox, 1);

    /* ---------- Backup ---------- */
    QGroupBox *backupBox = new QGroupBox(tr("Backup"), right);
    QVBoxLayout *backupLayout = new QVBoxLayout(backupBox);

    backupLayout->addWidget(new QLabel(tr("Backup base folder. A timestamped subfolder is created per run."), backupBox));

    QHBoxLayout *backupRow = new QHBoxLayout();
    m_backupFolderEdit = new QLineEdit(backupBox);
    m_backupBrowseBtn = new QPushButton(tr("Browse…"), backupBox);

    backupRow->addWidget(m_backupFolderEdit, 1);
    backupRow->addWidget(m_backupBrowseBtn);

    backupLayout->addLayout(backupRow);
    rightLayout->addWidget(backupBox);

    /* ===================== Final layout ===================== */
    QVBoxLayout *rootLayout = new QVBoxLayout(m_settingsTab);
    rootLayout->addWidget(splitter);

    m_tabs->addTab(m_settingsTab, themedIcon(QStringLiteral("settings")), tr("Settings"));

    /* ===================== Data + wiring ===================== */
    loadProjectFoldersForSettingsTab();
    setSettingsPanelEnabled(false);

    connect(m_projectFoldersList, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem *current, QListWidgetItem *)
            {
                if (!current)
                {
                    setSettingsPanelEnabled(false);
                    return;
                }

                const QString path = QDir(current->text()).absolutePath();

                // User action → activate project
                setActiveProject(path, /*updateSettingsSelection=*/false);

                setSettingsPanelEnabled(true);
                loadSettingsForProject(path);
            });
    /* --------- NEW: Project folders --------- */
    connect(m_addProjectFolderBtn, &QPushButton::clicked, this, &MainWindow::onBrowseProjectFolder);

    connect(m_removeProjectFolderBtn, &QPushButton::clicked, this, &MainWindow::onRemoveProjectFolder);

    /* --------- NEW: Exclude folders --------- */
    connect(m_addExcludeBrowseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseExcludeFolder);

    connect(m_removeExcludeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveExcludeFolder);

    /* --------- Backup browse (existing) --------- */
    connect(m_backupBrowseBtn, &QPushButton::clicked, this,
            [this]()
            {
                const QString projectRoot = currentSettingsProject();
                if (projectRoot.trimmed().isEmpty()) return;

                const QString start = !m_backupFolderEdit->text().trimmed().isEmpty() ? m_backupFolderEdit->text().trimmed() : projectRoot;

                const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Backup Folder"), start, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

                if (dir.isEmpty()) return;

                m_backupFolderEdit->setText(QDir(dir).absolutePath());
                saveSettingsForProject(projectRoot);
            });
}

/*!
 * ********************************************************************************************************************************
 * @brief        Adds a project folder to the Settings tab.
 *********************************************************************************************************************************/
void MainWindow::onBrowseProjectFolder()
{
    const QString start = !m_openProjectRoot.isEmpty() ? m_openProjectRoot : QDir::homePath();

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Project Folder"), start, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) return;

    const QString abs = QDir(dir).absolutePath();

    if (m_projectFoldersList->findItems(abs, Qt::MatchExactly).isEmpty())
    {
        auto *item = new QListWidgetItem(abs, m_projectFoldersList);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
}

/*!
 * ********************************************************************************************************************************
 * @brief        Removes the selected project folder from the Settings tab.
 *********************************************************************************************************************************/
void MainWindow::onRemoveProjectFolder()
{
    qDeleteAll(m_projectFoldersList->selectedItems());
    saveProjectFoldersFromSettingsTab();
}

/*!
 * ********************************************************************************************************************************
 * @brief        Saves the project folder list from the Settings tab to QSettings.
 *********************************************************************************************************************************/
void MainWindow::saveProjectFoldersFromSettingsTab() const
{
    if (!m_projectFoldersList) return;

    QStringList folders;
    folders.reserve(m_projectFoldersList->count());

    for (int i = 0; i < m_projectFoldersList->count(); ++i)
    {
        const QString path = m_projectFoldersList->item(i)->text().trimmed();
        if (!path.isEmpty() && !folders.contains(path)) folders.append(QDir(path).absolutePath());
    }

    QSettings s;
    s.setValue(QString::fromLatin1(kSettingsProjectFolders), folders);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Adds an excluded folder to the current project.
 *********************************************************************************************************************************/
void MainWindow::onBrowseExcludeFolder()
{
    const QString projectRoot = currentSettingsProject();
    if (projectRoot.trimmed().isEmpty()) return;
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Select Folder to Exclude"), projectRoot, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;
    const QString rel = QDir(projectRoot).relativeFilePath(dir);
    if (m_excludeFoldersList->findItems(rel, Qt::MatchExactly).isEmpty())
    {
        auto *item = new QListWidgetItem(rel, m_excludeFoldersList);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    saveSettingsForProject(projectRoot);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Removes selected excluded folders from the Settings tab.
 *********************************************************************************************************************************/
void MainWindow::onRemoveExcludeFolder()
{
    qDeleteAll(m_excludeFoldersList->selectedItems());
    const QString projectRoot = currentSettingsProject();
    if (!projectRoot.isEmpty()) saveSettingsForProject(projectRoot);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Returns the currently selected project root in the Settings tab.
 * @return       Absolute path to the selected project, or empty string if none selected.
 *********************************************************************************************************************************/
QString MainWindow::currentSettingsProject() const
{
    if (!m_projectFoldersList || !m_projectFoldersList->currentItem()) return {};
    return QDir(m_projectFoldersList->currentItem()->text()).absolutePath();
}

/*!
 * ********************************************************************************************************************************
 * @brief        Parses a user-entered extension list into a normalized QStringList.
 *
 * @details      Accepts comma, space, or semicolon separated text and normalizes
 *               extensions by trimming whitespace, removing leading dots, and
 *               converting to lowercase.
 *
 * @param[in]    text Raw text from the include-extensions QLineEdit.
 * @return       List of normalized file extensions (without dots).
 *********************************************************************************************************************************/
QStringList MainWindow::parseExtensionsText(const QString &text)
{
    QString normalized = text;
    normalized.replace(';', ',');
    normalized.replace(' ', ',');

    const QStringList parts = normalized.split(',', Qt::SkipEmptyParts);

    QStringList out;
    for (const QString &p : parts)
    {
        QString e = p.trimmed();
        if (e.startsWith('.')) e.remove(0, 1);

        e = e.toLower();

        if (!e.isEmpty() && !out.contains(e)) out.append(e);
    }

    return out;
}

/*!
 * ********************************************************************************************************************************
 * @brief        Saves Settings‑tab values for the specified project to QSettings.
 *********************************************************************************************************************************/
void MainWindow::saveSettingsForProject(const QString &projectRoot) const
{
    if (projectRoot.trimmed().isEmpty()) return;
    const QString id = psd::paths::projectIdForPath(projectRoot);
    QSettings s;
    /* ---------- Include extensions ---------- */
    QStringList includeExts = defaultIncludeExts();
    if (m_includeExtEdit)
    {
        const QString text = m_includeExtEdit->text().trimmed();
        if (!text.isEmpty()) includeExts = parseExtensionsText(text);
    }
    /* ---------- Exclude folders ---------- */
    QStringList excludeDirs;
    if (m_excludeFoldersList)
    {
        for (int i = 0; i < m_excludeFoldersList->count(); ++i)
        {
            const QString v = m_excludeFoldersList->item(i)->text().trimmed();
            if (!v.isEmpty() && !excludeDirs.contains(v)) excludeDirs.append(v);
        }
    }
    if (excludeDirs.isEmpty()) excludeDirs = defaultExcludeDirs();
    /* ---------- Backup folder ---------- */
    QString backupFolder;
    if (m_backupFolderEdit) backupFolder = QDir(m_backupFolderEdit->text().trimmed()).absolutePath();
    /* ---------- Persist ---------- */
    s.setValue(QStringLiteral("project/%1/includeExts").arg(id), includeExts);
    s.setValue(QStringLiteral("project/%1/excludeDirs").arg(id), excludeDirs);
    s.setValue(QStringLiteral("project/%1/backupFolder").arg(id), backupFolder);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Enables or disables all Settings‑panel controls.
 *********************************************************************************************************************************/
void MainWindow::setSettingsPanelEnabled(bool enabled)
{
    if (m_includeExtEdit)      m_includeExtEdit->setEnabled(enabled);
    if (m_excludeFoldersList)  m_excludeFoldersList->setEnabled(enabled);
    if (m_addExcludeBrowseBtn) m_addExcludeBrowseBtn->setEnabled(enabled);
    if (m_removeExcludeBtn)    m_removeExcludeBtn->setEnabled(enabled);
    if (m_backupFolderEdit)    m_backupFolderEdit->setEnabled(enabled);
    if (m_backupBrowseBtn)     m_backupBrowseBtn->setEnabled(enabled);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Loads per‑project Settings tab values from QSettings.
 *********************************************************************************************************************************/
void MainWindow::loadSettingsForProject(const QString &projectRoot)
{
    if (projectRoot.trimmed().isEmpty()) return;

    QSettings s;
    const QString id = psd::paths::projectIdForPath(projectRoot);

    QStringList includeExts = s.value(QStringLiteral("project/%1/includeExts").arg(id), defaultIncludeExts()).toStringList();

    QStringList excludeDirs = s.value(QStringLiteral("project/%1/excludeDirs").arg(id), defaultExcludeDirs()).toStringList();

    /* ---------- NEW: merge .gitignore ---------- */
    const QStringList gitignoreDirs = excludeDirsFromGitignore(projectRoot);
    for (const QString &d : gitignoreDirs)
    {
        if (!excludeDirs.contains(d)) excludeDirs.append(d);
    }

    const QString backupFolder = s.value(QStringLiteral("project/%1/backupFolder").arg(id)).toString();

    if (m_includeExtEdit) m_includeExtEdit->setText(includeExts.join(QStringLiteral(", ")));

    if (m_excludeFoldersList)
    {
        m_excludeFoldersList->blockSignals(true);
        m_excludeFoldersList->clear();

        for (const QString &ex : std::as_const(excludeDirs))
        {
            auto *item = new QListWidgetItem(ex, m_excludeFoldersList);
            item->setFlags(item->flags() | Qt::ItemIsEditable);
        }

        m_excludeFoldersList->blockSignals(false);
    }

    if (m_backupFolderEdit) m_backupFolderEdit->setText(backupFolder);
}

/*!
 * ****************************************************************************************************************************
 * @brief   Handles Tools → Create Project‑Source.txt.
 * @details Validates required Settings values before invoking ProjectSourceExporter.
 *****************************************************************************************************************************/
void MainWindow::onProjectSourceTriggered()
{
    if (m_statusQueue)
        m_statusQueue->enqueue(tr("Project Source: starting..."), 2);
    else
        statusBar()->showMessage(tr("Project Source: starting..."), 2000);

    const QString projectRoot = m_activeProjectRoot;
    if (projectRoot.trimmed().isEmpty() || !QDir(projectRoot).exists())
    {
        appendToLog(tr("ERROR: No active project selected.\n"));
        return;
    }

    /* ------------------------------------------------------------------------------------------------
     * Validate Settings → Backup folder (required)
     * ------------------------------------------------------------------------------------------------ */
    if (!m_backupFolderEdit || m_backupFolderEdit->text().trimmed().isEmpty())
    {
        if (m_tabs) m_tabs->setCurrentIndex(1); // Settings tab
        if (m_backupFolderEdit) m_backupFolderEdit->setFocus();

        if (m_statusQueue)
            m_statusQueue->enqueue(tr("Please set a Backup folder in Settings before generating."), 5);
        else
            statusBar()->showMessage(tr("Please set a Backup folder in Settings before generating."), 5000);

        appendToLog(tr("ERROR: Backup folder not set. Generation aborted.\n"));
        return;
    }

    /* ------------------------------------------------------------------------------------------------
     * Debug log (existing behavior)
     * ------------------------------------------------------------------------------------------------ */
    {
        QFile f(QDir(projectRoot).filePath(QStringLiteral("ProjectSource-Debug.log")));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        {
            qWarning() << "Failed to open debug log file:" << f.fileName();
            return;
        }

        QTextStream ts(&f);
        ts << "=== ProjectSource Debug Log ===\n";
        ts << "Active project root: " << projectRoot << "\n";
    }

    QString procLog;
    const QString outPath = ProjectSourceExporter::generateAndReturnPath(this, statusBar(), projectRoot, &procLog);

    if (!procLog.trimmed().isEmpty() && m_logView)
    {
        appendToLog(tr("\n==================== [Project Source Output] ====================\n"));
        appendToLog(procLog);
    }

    if (outPath.isEmpty())
    {
        statusBar()->showMessage(tr("Project Source: failed."), 4000);
        return;
    }

    showFileInLogTab(outPath, tr("Project-Source.txt (%1)").arg(outPath));
}

/*! ******************************************************************************************************************************
 * @brief       Loads the current application project context.
 * @details     This function intentionally performs no runtime source scanning
 *              or tree construction. The application is not a runtime analyzer.
 *
 *              All source inspection and tree generation is performed by
 *              ProjectSourceExporter when explicitly requested by the user.
 *
 * @param[in]   projectRoot Absolute path to the project root.
 ******************************************************************************************************************************* */
void MainWindow::loadProjectTree(const QString& projectRoot)
{
    if (projectRoot.isEmpty())
    {
        appendToLog(tr("ERROR: Project root is empty.\n"));
        return;
    }

    if (!QDir(projectRoot).exists())
    {
        appendToLog(tr("ERROR: Project root does not exist:\n%1\n").arg(projectRoot));
        return;
    }

    // Informational only — this app does not analyze project trees at runtime.
    appendToLog(tr("Project root loaded:\n%1\n").arg(projectRoot));
    appendToLog(tr("Note: Runtime source analysis is not performed by this application.\n"));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Show the About dialog.
 * @details Displays application information in a modal dialog.
 *****************************************************************************************************************************/
void MainWindow::showAboutDialog()
{
    AboutDialog dlg(this);
    dlg.exec();
}

/*!
 * ********************************************************************************************************************************
 * @file         MainWindow.cpp
 * @brief        End of MainWindow.cpp.
 *********************************************************************************************************************************/
