/*!
 * ********************************************************************************************************************************
 * @file        AboutDialog.cpp
 * @brief       About dialog implementation.
 * @details     Displays application information and troubleshooting diagnostics in a modal dialog
 *              without UI files. Provides a copy button to copy all diagnostics to the clipboard.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-04
 *********************************************************************************************************************************/

#include "AboutDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QLabel>
#include <QPlainTextEdit>
#include <QStorageInfo>
#include <QSysInfo>
#include <QVBoxLayout>

#include <QDir>
#include <QFile>
#include <QPushButton>
#include <QTextStream>

#if defined(Q_OS_WIN)
#include <windows.h>
#endif

#if defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

/*!
 * ****************************************************************************************************************************
 * @brief   Convert a byte count to a human-readable string.
 * @details Uses base-1024 units (KiB, MiB, GiB, TiB) and two decimals for readability.
 * @param   bytes Byte count.
 * @return  Human-readable size string.
 *****************************************************************************************************************************/
static QString formatBytes(qulonglong bytes)
{
    static const char *kUnits[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    double value = static_cast<double>(bytes);
    int unit = 0;

    while (value >= 1024.0 && unit < 5)
    {
        value /= 1024.0;
        ++unit;
    }

    if (unit == 0) return QStringLiteral("%1 %2").arg(static_cast<qulonglong>(value)).arg(QString::fromLatin1(kUnits[unit]));
    return QStringLiteral("%1 %2").arg(value, 0, 'f', 2).arg(QString::fromLatin1(kUnits[unit]));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Read the first line of a text file if it exists.
 * @param   path Absolute file path.
 * @return  First line of file or empty string.
 *****************************************************************************************************************************/
static QString readFirstLine(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    QTextStream in(&file);
    return in.readLine().trimmed();
}

/*!
 * ****************************************************************************************************************************
 * @brief   Detect whether the app is running under WSL (Windows Subsystem for Linux).
 * @details Detection is Linux-only. Returns "Yes" with optional distro info when detected.
 * @return  WSL status string ("Yes ..."/"No") for Linux, otherwise empty string.
 *****************************************************************************************************************************/
static QString wslStatus()
{
#if defined(Q_OS_LINUX)
    const QString distro = qEnvironmentVariable("WSL_DISTRO_NAME");
    if (!distro.isEmpty()) return QStringLiteral("Yes (%1)").arg(distro);

    const QString osRelease = readFirstLine(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (osRelease.contains(QStringLiteral("microsoft"), Qt::CaseInsensitive)) return QStringLiteral("Yes");

    const QString versionLine = readFirstLine(QStringLiteral("/proc/version"));
    if (versionLine.contains(QStringLiteral("microsoft"), Qt::CaseInsensitive)) return QStringLiteral("Yes");

    return QStringLiteral("No");
#else
    return {};
#endif
}

/*!
 * ****************************************************************************************************************************
 * @brief   Get total and available system memory.
 * @details Uses platform-specific APIs for Windows/macOS and /proc/meminfo on Linux.
 * @param   totalBytes Output total physical memory in bytes.
 * @param   availBytes Output available/free memory in bytes (best-effort).
 *****************************************************************************************************************************/
static void systemMemory(qulonglong &totalBytes, qulonglong &availBytes)
{
    totalBytes = 0;
    availBytes = 0;

#if defined(Q_OS_WIN)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex))
    {
        totalBytes = static_cast<qulonglong>(statex.ullTotalPhys);
        availBytes = static_cast<qulonglong>(statex.ullAvailPhys);
    }
#elif defined(Q_OS_MACOS)
    // Total memory
    uint64_t memsize = 0;
    size_t len = sizeof(memsize);
    if (sysctlbyname("hw.memsize", &memsize, &len, nullptr, 0) == 0) totalBytes = static_cast<qulonglong>(memsize);

           // Free memory (best-effort)
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    mach_port_t host = mach_host_self();

    if (host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmstat), &count) == KERN_SUCCESS)
    {
        vm_size_t pageSize = 0;
        host_page_size(host, &pageSize);
        availBytes = static_cast<qulonglong>(vmstat.free_count) * static_cast<qulonglong>(pageSize);
    }
#elif defined(Q_OS_LINUX)
    // Parse /proc/meminfo for MemTotal and MemAvailable (both in kB)
    QFile file(QStringLiteral("/proc/meminfo"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    qulonglong memTotalKb = 0;
    qulonglong memAvailKb = 0;

    while (!in.atEnd())
    {
        const QString line = in.readLine();
        if (line.startsWith(QStringLiteral("MemTotal:"), Qt::CaseInsensitive))
        {
            const QStringList parts = line.split(QChar(' '), Qt::SkipEmptyParts);
            if (parts.size() >= 2) memTotalKb = parts.at(1).toULongLong();
        }
        else if (line.startsWith(QStringLiteral("MemAvailable:"), Qt::CaseInsensitive))
        {
            const QStringList parts = line.split(QChar(' '), Qt::SkipEmptyParts);
            if (parts.size() >= 2) memAvailKb = parts.at(1).toULongLong();
        }

        if (memTotalKb > 0 && memAvailKb > 0) break;
    }

    totalBytes = memTotalKb * 1024ULL;
    availBytes = memAvailKb * 1024ULL;
#endif
}

/*!
 * ****************************************************************************************************************************
 * @brief   Build a diagnostic report for troubleshooting.
 * @details Includes application, Qt/runtime, OS/kernel, WSL (when applicable), storage, and memory.
 * @return  Multi-line diagnostics report.
 *****************************************************************************************************************************/
static QString buildDiagnostics()
{
    const QString appName = QApplication::applicationName();
    const QString appVersion = QApplication::applicationVersion();

    QStringList lines;
    lines << QStringLiteral("%1").arg(appName);
    lines << QStringLiteral("Version: %1").arg(appVersion.isEmpty() ? QStringLiteral("(unknown)") : appVersion);
    lines << QStringLiteral("Qt (runtime): %1").arg(QString::fromLatin1(qVersion()));
    lines << QStringLiteral("Qt (build): %1").arg(QString::fromLatin1(QT_VERSION_STR));
    lines << QStringLiteral("Build ABI: %1").arg(QSysInfo::buildAbi());
    lines << QStringLiteral("CPU: %1").arg(QSysInfo::currentCpuArchitecture());
    lines << QStringLiteral("Kernel: %1 %2").arg(QSysInfo::kernelType(), QSysInfo::kernelVersion());
    lines << QStringLiteral("OS: %1").arg(QSysInfo::prettyProductName());
    lines << QStringLiteral("Hostname: %1").arg(QSysInfo::machineHostName());
    lines << QStringLiteral("App path: %1").arg(QCoreApplication::applicationFilePath());
    lines << QStringLiteral("Working dir: %1").arg(QDir::currentPath());

#if defined(Q_OS_LINUX)
    lines << QStringLiteral("WSL: %1").arg(wslStatus());
#endif

           // Storage info (root + home)
    const QStorageInfo rootStore(QDir::rootPath());
    if (rootStore.isValid() && rootStore.isReady())
    {
        lines << QStringLiteral("Disk (root) total: %1").arg(formatBytes(static_cast<qulonglong>(rootStore.bytesTotal())));
        lines << QStringLiteral("Disk (root) free: %1").arg(formatBytes(static_cast<qulonglong>(rootStore.bytesFree())));
        lines << QStringLiteral("Disk (root) available: %1").arg(formatBytes(static_cast<qulonglong>(rootStore.bytesAvailable())));
        lines << QStringLiteral("Disk (root) fs: %1").arg(QString::fromUtf8(rootStore.fileSystemType()));
    }

    const QStorageInfo homeStore(QDir::homePath());
    if (homeStore.isValid() && homeStore.isReady())
    {
        lines << QStringLiteral("Disk (home) path: %1").arg(homeStore.rootPath());
        lines << QStringLiteral("Disk (home) total: %1").arg(formatBytes(static_cast<qulonglong>(homeStore.bytesTotal())));
        lines << QStringLiteral("Disk (home) free: %1").arg(formatBytes(static_cast<qulonglong>(homeStore.bytesFree())));
        lines << QStringLiteral("Disk (home) available: %1").arg(formatBytes(static_cast<qulonglong>(homeStore.bytesAvailable())));
        lines << QStringLiteral("Disk (home) fs: %1").arg(QString::fromUtf8(homeStore.fileSystemType()));
    }

           // Memory info
    qulonglong totalMem = 0;
    qulonglong availMem = 0;
    systemMemory(totalMem, availMem);

    if (totalMem > 0) lines << QStringLiteral("Memory total: %1").arg(formatBytes(totalMem));
    if (availMem > 0) lines << QStringLiteral("Memory available: %1").arg(formatBytes(availMem));

    return lines.join(QLatin1Char('\n'));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Construct AboutDialog.
 * @details Builds a modal About dialog with a diagnostics text box and a copy button.
 * @param   parent Optional parent widget.
 *****************************************************************************************************************************/
AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("About"));
    setModal(true);
    setMinimumWidth(520);

    const QString appName = QApplication::applicationName();
    const QString appVersion = QApplication::applicationVersion();

    QLabel *titleLabel = new QLabel(QStringLiteral("%1").arg(appName), this);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleLabel->setFont(titleFont);

    QLabel *versionLabel = new QLabel(tr("Version %1").arg(appVersion.isEmpty() ? QStringLiteral("(unknown)") : appVersion), this);

    titleLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setAlignment(Qt::AlignCenter);

    QPlainTextEdit *infoBox = new QPlainTextEdit(this);
    infoBox->setReadOnly(true);
    infoBox->setUndoRedoEnabled(false);
    infoBox->setWordWrapMode(QTextOption::NoWrap);
    infoBox->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    infoBox->setPlainText(buildDiagnostics());

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    QPushButton *copyButton = buttons->addButton(tr("Copy"), QDialogButtonBox::ActionRole);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(copyButton, &QPushButton::clicked, this,
            [infoBox]()
            {
                QClipboard *cb = QGuiApplication::clipboard();
                if (cb) cb->setText(infoBox->toPlainText());
            });

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addSpacing(10);
    layout->addWidget(infoBox, 1);
    layout->addWidget(buttons);

    setLayout(layout);
}

/*!
 * ********************************************************************************************************************************
 * @brief End of AboutDialog.cpp
 *********************************************************************************************************************************/
