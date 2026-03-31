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
 * @author       Jeffrey Scott Flesher
 * @date         2026-03-31
 *********************************************************************************************************************************/

#include "MainWindow.h"

#include <QAction>
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
#include <QVBoxLayout>

#include "AppLogUtils.h"
#include "EmbeddedDocs.h"
#include "ProjectSourceExporter.h"
#include "StatusBarQueue.h"

/* ------------------------------------------------------------------------------------------------
 * Persistent settings keys
 * ------------------------------------------------------------------------------------------------ */
static constexpr const char *kSettingsLastFolder = "ui/lastProjectFolder";
static constexpr const char *kSettingsRecentList = "ui/recentProjectFolders";
static constexpr const char *kSettingsProjectFolders = "ui/projectFolders";
static constexpr int kMaxRecentProjects = 10;

/*!
 * ********************************************************************************************************************************
 * @brief        Constructs the main application window.
 *********************************************************************************************************************************/
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle(tr("ProjectSource"));
    resize(1100, 700);

    psd::log::clearLogFilesAtStartup();

    loadRecentProjects();

    createActions();
    createMenus();
    createToolbar();
    createTabs();
    createStatusBar();
}

/*!
 * ********************************************************************************************************************************
 * @brief        Destroys the main application window.
 *********************************************************************************************************************************/
MainWindow::~MainWindow() = default;

/*!
 * ********************************************************************************************************************************
 * @brief        Returns a theme‑aware icon from application resources.
 *********************************************************************************************************************************/
QIcon MainWindow::themedIcon(const QString &baseName) const
{
    const bool dark = qApp->palette().color(QPalette::Window).lightness() < 128;

    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");

    return QIcon(QStringLiteral(":/icons/%1/%2.svg").arg(theme, baseName));
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
 * ********************************************************************************************************************************
 * @brief        Rebuilds the File → Open Recent menu.
 *********************************************************************************************************************************/
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
        QAction *a = m_recentMenu->addAction(themedIcon("doc"), path);
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
 * ********************************************************************************************************************************
 * @brief        Creates application actions.
 *********************************************************************************************************************************/
void MainWindow::createActions()
{
    m_openAction = new QAction(themedIcon("doc"), tr("&Open…"), this);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::onOpenTriggered);

    m_projectSourceAction = new QAction(themedIcon("update"), tr("Create Project‑Source.txt"), this);
    connect(m_projectSourceAction, &QAction::triggered, this, &MainWindow::onProjectSourceTriggered);

    m_exitAction = new QAction(themedIcon("exit"), tr("E&xit"), this);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    m_usageAction = new QAction(themedIcon("help"), tr("Usage"), this);
    connect(m_usageAction, &QAction::triggered, this, [this]() { psd::docs::showUsageDialog(this); });

    m_aboutAction = new QAction(themedIcon("help"), tr("About"), this);
    connect(m_aboutAction, &QAction::triggered, this, [this]() { statusBar()->showMessage( tr("ProjectSource — Generate Project‑Source.txt from a project."), 4000); });
}

/*!
 * ********************************************************************************************************************************
 * @brief        Creates the menu bar.
 *********************************************************************************************************************************/
void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(m_openAction);

    m_recentMenu = fileMenu->addMenu(tr("Open &Recent"));
    rebuildRecentProjectsMenu();

    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);

    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(m_projectSourceAction);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(m_usageAction);
    helpMenu->addAction(m_aboutAction);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Creates the main toolbar.
 *********************************************************************************************************************************/
void MainWindow::createToolbar()
{
    m_toolbar = addToolBar(tr("Main Toolbar"));
    m_toolbar->setMovable(false);

    m_toolbar->addAction(m_openAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_projectSourceAction);
    m_toolbar->addSeparator();
    m_toolbar->addAction(m_exitAction);
}

/*!
 * ********************************************************************************************************************************
 * @brief        Creates the central tab widget (Log + Settings).
 *********************************************************************************************************************************/
void MainWindow::createTabs()
{
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    /* ------------------------------ Log tab ------------------------------ */
    QWidget *logTab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(logTab);

    m_logProgress = new QProgressBar(logTab);
    m_logProgress->setRange(0, 100);
    layout->addWidget(m_logProgress);

    m_logView = new QTextEdit(logTab);
    m_logView->setReadOnly(true);
    m_logView->setLineWrapMode(QTextEdit::NoWrap);
    layout->addWidget(m_logView, 1);

    m_tabs->addTab(logTab, themedIcon("log"), tr("Log"));

    /* ---------------------------- Settings tab ---------------------------- */
    createSettingsTab();
}

/*!
 * ********************************************************************************************************************************
 * @brief        Creates the status bar.
 *********************************************************************************************************************************/
void MainWindow::createStatusBar()
{
    m_statusQueue = new StatusBarQueue(statusBar(), this);
    m_statusQueue->enqueue(tr("Ready"), 2);
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

    m_openProjectRoot = abs;
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
 * ********************************************************************************************************************************
 * @brief        Handles File → Open.
 *********************************************************************************************************************************/
void MainWindow::onOpenTriggered()
{
    const QString start = !m_openProjectRoot.isEmpty() ? m_openProjectRoot : QDir::homePath();

    const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Project Folder"), start, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) return;

    openProjectFolder(dir, true);
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
 * ********************************************************************************************************************************
 * @brief        Computes a stable project identifier for settings storage.
 *********************************************************************************************************************************/
QString MainWindow::projectIdForPath(const QString &absPath)
{
    const QByteArray hash = QCryptographicHash::hash(absPath.toUtf8(), QCryptographicHash::Md5);

    return QString::fromLatin1(hash.toHex());
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
 * ********************************************************************************************************************************
 * @brief        Returns default excluded directory patterns.
 *********************************************************************************************************************************/
QStringList MainWindow::defaultExcludeDirs()
{
    return {QStringLiteral("build*"),   QStringLiteral(".rcc"),       QStringLiteral("CMakeFiles"),
            QStringLiteral(".git"),     QStringLiteral(".qtcreator"), QStringLiteral("engines"),
            QStringLiteral("AppDir"),  QStringLiteral("0-Archive")};
}

/*!
 * ********************************************************************************************************************************
 * @brief   Extracts excluded directory rules from a project's .gitignore file.
 *
 * @details Reads the `.gitignore` file located at the specified project root and
 *          returns a list of directory exclusion rules suitable for directory
 *          filtering. The parser:
 *            - Skips empty lines and comments
 *            - Ignores negation rules (!)
 *            - Accepts only directory rules ending with '/'
 *            - Rejects file-extension and single-file rules
 *            - Normalizes trailing slashes and glob suffixes
 *
 * @param[in] projectRoot Absolute path to the project root directory.
 *
 * @return  A list of relative directory paths to exclude, with no trailing slashes.
 * *******************************************************************************************************************************/
QStringList MainWindow::excludeDirsFromGitignore(const QString &projectRoot)
{
    QStringList result;

    const QString gitignorePath =
        QDir(projectRoot).filePath(QStringLiteral(".gitignore"));

    QFile file(gitignorePath);
    if (!file.exists() ||
        !file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;

    QTextStream in(&file);

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

               // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith('#'))
            continue;

               // Ignore negation rules
        if (line.startsWith('!'))
            continue;

               // ✅ Directory rules ONLY: must end with '/'
        if (!line.endsWith('/'))
            continue;

               // Remove trailing '/'
        line.chop(1);

               // Normalize gitignore-style "dir/*"
        if (line.endsWith("/*"))
            line.chop(2);

        if (!result.contains(line))
            result.append(line);
    }

    return result;
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
    const QString id = projectIdForPath(projectRoot);
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
    const QString id = projectIdForPath(projectRoot);

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

/* =============================================================================================================================
 * Project‑Source generation entry point
 * =============================================================================================================================
 */

/*!
 * ********************************************************************************************************************************
 * @brief        Handles Tools → Create Project‑Source.txt.
 *********************************************************************************************************************************/
void MainWindow::onProjectSourceTriggered()
{
    if (m_openProjectRoot.trimmed().isEmpty())
    {
        onOpenTriggered();

        if (m_openProjectRoot.trimmed().isEmpty())
        {
            if (m_statusQueue) m_statusQueue->enqueue(tr("No project folder selected."), 3);
            return;
        }
    }
    const QString cmakeListsPath = QDir(m_openProjectRoot).filePath(QStringLiteral("CMakeLists.txt"));
    if (!QFileInfo::exists(cmakeListsPath))
    {
        QMessageBox::warning(this, tr("Project Source"), tr("Selected folder does not contain CMakeLists.txt:\n\n%1").arg(m_openProjectRoot));
        if (m_statusQueue) m_statusQueue->enqueue(tr("Selected folder is not a CMake project."), 5);
        return;
    }
    QSettings s;
    const QString id = projectIdForPath(m_openProjectRoot);
    const QStringList includeExts = s.value(QStringLiteral("project/%1/includeExts").arg(id), defaultIncludeExts()).toStringList();
    const QStringList excludeDirs = s.value(QStringLiteral("project/%1/excludeDirs").arg(id), defaultExcludeDirs()).toStringList();
    const QString backupBase = s.value(QStringLiteral("project/%1/backupFolder").arg(id)).toString();
    beginLogReset();
    QString backupLog;
    const QString backupPath = ProjectSourceExporter::backupProjectToTimestampedFolder(this, statusBar(), m_openProjectRoot, excludeDirs, backupBase, &backupLog);

    if (!backupLog.trimmed().isEmpty())
    {
        appendToLog(tr("==================================== [Backup] ====================================\n"));
        appendToLog(backupLog);
        appendToLog("\n");
    }
    if (backupPath.isEmpty())
    {
        appendToLog(tr("WARNING: Backup did not complete successfully. Proceeding anyway.\n\n"));
    }

    QString processLog;
    const QString outPath = ProjectSourceExporter::generateAndReturnPath(this, statusBar(), m_openProjectRoot, includeExts, excludeDirs, &processLog);

    if (!processLog.trimmed().isEmpty())
    {
        appendToLog(tr("================================ [Process Output] ================================\n"));
        appendToLog(processLog);
        appendToLog("\n");
    }

    if (outPath.isEmpty())
    {
        appendToLog(tr("ERROR: Project source generation failed.\n"));
        if (m_statusQueue) m_statusQueue->enqueue(tr("Project Source failed."), 4);
        return;
    }

    showFileInLogTab(outPath, tr("Project‑Source.txt (%1)").arg(outPath));
}

/*!
 * ********************************************************************************************************************************
 * @file         MainWindow.cpp
 * @brief        End of MainWindow.cpp.
 *********************************************************************************************************************************/
