/*!
 * ********************************************************************************************************************************
 * @file        main.cpp
 * @brief       Application entry point for ProjectSourceDoc.
 * @details
 *  Initializes the Qt application, installs runtime event filters, applies theme-aware resources,
 *  loads translations, resolves VERSION from the project root, and launches the main UI.
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>
#include <QPalette>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
#include <QWidget>
#include <QtGlobal>

/*!
 * ********************************************************************************************************************************
 * @brief Configure early runtime (before QApplication).
 *
 * This function is restricted to environment configuration only.
 * It must NOT call any QCoreApplication / QApplication APIs.
 *********************************************************************************************************************************/
static void configureEarlyRuntime(int argc, char *argv[])
{
#ifdef __MINGW32__
    qputenv("QT_DEFAULT_RHI", "opengl");
    qputenv("QT_QPA_PLATFORM", "windows");

    const QString exeDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    const QString pluginsDir = QDir(exeDir).filePath(QStringLiteral("plugins"));
    if (QDir(pluginsDir).exists()) qputenv("QT_PLUGIN_PATH", QFile::encodeName(pluginsDir));

    fprintf(stderr, "QT_DEFAULT_RHI=%s\n", qgetenv("QT_DEFAULT_RHI").constData());
    fprintf(stderr, "QT_QPA_PLATFORM=%s\n", qgetenv("QT_QPA_PLATFORM").constData());
    fprintf(stderr, "QT_PLUGIN_PATH=%s\n", qgetenv("QT_PLUGIN_PATH").constData());
#else
    Q_UNUSED(argc);
    Q_UNUSED(argv);
#endif
}

/*!
 * ********************************************************************************************************************************
 * @brief Locate the project root directory.
 * @details Walks upward from the executable directory until CMakeLists.txt is found.
 * @return  Absolute path to project root, or empty string if not found.
 *********************************************************************************************************************************/
static QString findProjectRoot()
{
    QDir dir(QCoreApplication::applicationDirPath());

    while (dir.exists())
    {
        if (QFileInfo::exists(dir.filePath(QStringLiteral("CMakeLists.txt")))) return dir.absolutePath();
        if (!dir.cdUp()) break;
    }

    return {};
}

/*!
 * ********************************************************************************************************************************
 * @brief Load application version from VERSION file.
 * @details VERSION is the single source of truth per RELEASE.md.
 * @return  Version string, or "0.0.0" if unavailable.
 *********************************************************************************************************************************/
static QString loadVersionFromFile()
{
    const QString projectRoot = findProjectRoot();
    if (projectRoot.isEmpty())
    {
        qWarning() << "Project root not found; VERSION unavailable";
        return QStringLiteral("0.0.0");
    }

    const QString versionPath = QDir(projectRoot).filePath(QStringLiteral("VERSION"));
    QFile file(versionPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "VERSION file not readable:" << versionPath;
        return QStringLiteral("0.0.0");
    }

    QTextStream in(&file);
    const QString version = in.readLine().trimmed();
    if (version.isEmpty())
    {
        qWarning() << "VERSION file empty:" << versionPath;
        return QStringLiteral("0.0.0");
    }

    qDebug() << "Loaded VERSION:" << version << "from" << versionPath;
    return version;
}

/*!
 * ********************************************************************************************************************************
 * @brief Returns true when the active application palette implies a dark theme.
 *********************************************************************************************************************************/
static bool isDarkTheme()
{
    return qApp->palette().color(QPalette::Window).lightness() < 128;
}

/*!
 * ********************************************************************************************************************************
 * @brief Applies the theme-aware application icon.
 *
 * NOTE: This function MUST only be called after the event loop has started.
 *********************************************************************************************************************************/
static void applyThemedAppIcon()
{
    const QString theme = isDarkTheme() ? QStringLiteral("dark") : QStringLiteral("light");
    const QString path = QStringLiteral(":/icons/%1/app.svg").arg(theme);

    const QIcon icon(path);
    QApplication::setWindowIcon(icon);

    const auto widgets = QApplication::topLevelWidgets();
    for (QWidget *w : widgets) w->setWindowIcon(icon);
}

/*!
 * ********************************************************************************************************************************
 * @class ThemeWatcher
 * @brief Reapplies theme-aware resources on palette/style changes.
 *********************************************************************************************************************************/
class ThemeWatcher final : public QObject
{
    public:
        /*!
         * ****************************************************************************************************************************
         * @brief Construct ThemeWatcher.
         *****************************************************************************************************************************/
        explicit ThemeWatcher(QObject *parent = nullptr) : QObject(parent) {}

    protected:
        /*!
         * ****************************************************************************************************************************
         * @brief Event filter for theme-related changes.
         *****************************************************************************************************************************/
        bool eventFilter(QObject *obj, QEvent *ev) override
        {
            Q_UNUSED(obj);
            switch (ev->type())
            {
            case QEvent::ApplicationPaletteChange:
            case QEvent::ThemeChange:
            case QEvent::StyleChange:
                applyThemedAppIcon();
                break;
            default:
                break;
            }
            return false;
        }
};

/*!
 * ********************************************************************************************************************************
 * @brief Normalizes the system locale for translation selection.
 *********************************************************************************************************************************/
static QString normalizedLocale()
{
    const QString systemLocale = QLocale::system().name();
    if (systemLocale == QStringLiteral("zh_CN")) return QStringLiteral("zh_CN");
    return systemLocale.section(QLatin1Char('_'), 0, 0);
}

/*!
 * ********************************************************************************************************************************
 * @brief Loads the application translator from embedded resources.
 *********************************************************************************************************************************/
static bool loadAppTranslator(QTranslator &translator)
{
    const QString loc = normalizedLocale();
    if (translator.load(QStringLiteral(":/i18n/ProjectSourceDoc_%1.qm").arg(loc))) return true;
    if (translator.load(QStringLiteral(":/i18n/ProjectSourceDoc_en.qm"))) return true;
    return false;
}

/*!
 * ********************************************************************************************************************************
 * @brief Program entry point.
 *********************************************************************************************************************************/
int main(int argc, char *argv[])
{
    configureEarlyRuntime(argc, argv);

    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("ProjectSourceDoc"));
    QApplication::setOrganizationName(QStringLiteral("ProjectSourceDoc"));
    QApplication::setApplicationVersion(loadVersionFromFile());

    ThemeWatcher watcher(&app);
    app.installEventFilter(&watcher);

    QTimer::singleShot(0, &app,
                       [&app]()
                       {
                           static QTranslator translator;
                           if (loadAppTranslator(translator)) app.installTranslator(&translator);
                           applyThemedAppIcon();
                       });

    MainWindow window;
    window.show();

    return app.exec();
}

/*!
 * ********************************************************************************************************************************
 * @brief End of main.cpp
 *********************************************************************************************************************************/
