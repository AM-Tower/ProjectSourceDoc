/*!
 * ********************************************************************************************************************************
 * @file main.cpp
 * @brief Application entry point for ProjectSourceDoc.
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-03-31
 * @details
 *  This file initializes the Qt application object, installs runtime event filters, applies a theme-aware application icon,
 *  installs the application translator from embedded resources, and launches the main UI.
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
#include <QTimer>
#include <QTranslator>
#include <QWidget>
#include <QtGlobal>

/*!
 * ********************************************************************************************************************************
 * @brief Configure early runtime (before QApplication)
 *
 * This function is restricted to environment configuration only.
 * It must NOT call any QCoreApplication / QApplication APIs.
 *********************************************************************************************************************************/
static void configureEarlyRuntime(int argc, char *argv[])
{
    #ifdef __MINGW32__
    // Force OpenGL RHI on MinGW for stability (do NOT remove).
    qputenv("QT_DEFAULT_RHI", "opengl");
    qputenv("QT_QPA_PLATFORM", "windows");

    // Optional: allow plugins next to the executable (Windows portability).
    const QString exeDir = QFileInfo(QString::fromLocal8Bit(argv[0])).absolutePath();
    const QString pluginsDir = QDir(exeDir).filePath("plugins");
    if (QDir(pluginsDir).exists())
    {
        qputenv("QT_PLUGIN_PATH", QFile::encodeName(pluginsDir));
    }

    // Diagnostic output (stderr is safe pre-QApplication).
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
    const QString theme = isDarkTheme() ? "dark" : "light";
    const QString path = QString(":/icons/%1/app.svg").arg(theme);

    const QIcon icon(path);
    QApplication::setWindowIcon(icon);

    for (QWidget *w : QApplication::topLevelWidgets())
    {
        w->setWindowIcon(icon);
    }

    qDebug() << "App icon path:" << path << "exists?" << QFile::exists(path) << "isNull?" << icon.isNull();
}

/*!
 * ********************************************************************************************************************************
 * @class ThemeWatcher
 * @brief Reapplies theme-aware resources on palette/style changes.
 *********************************************************************************************************************************/
class ThemeWatcher final : public QObject
{
    public:
        explicit ThemeWatcher(QObject *parent = nullptr) : QObject(parent) {}

    protected:
        bool eventFilter(QObject *obj, QEvent *ev) override
        {
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
            return QObject::eventFilter(obj, ev);
        }
};

/*!
 * ********************************************************************************************************************************
 * @brief Normalizes the system locale for translation selection.
 *********************************************************************************************************************************/
static QString normalizedLocale()
{
    const QString systemLocale = QLocale::system().name();
    if (systemLocale == "zh_CN") return "zh_CN";
    return systemLocale.section('_', 0, 0);
}

/*!
 * ********************************************************************************************************************************
 * @brief Loads the application translator from embedded resources.
 *********************************************************************************************************************************/
static bool loadAppTranslator(QTranslator &translator)
{
    const QString loc = normalizedLocale();
    if (translator.load(QString(":/i18n/ProjectSourceDoc_%1.qm").arg(loc))) return true;
    if (translator.load(":/i18n/ProjectSourceDoc_en.qm")) return true;
    return false;
}

/*!
 * ********************************************************************************************************************************
 * @brief Program entry point.
 *********************************************************************************************************************************/
int main(int argc, char *argv[])
{
    // ✅ Environment setup only
    configureEarlyRuntime(argc, argv);

    // ✅ QApplication must be constructed as early as possible
    QApplication app(argc, argv);

    QApplication::setApplicationName("ProjectSourceDoc");
    QApplication::setOrganizationName("ProjectSourceDoc");
    QApplication::setApplicationVersion("0.1.0");

    // ✅ Install theme watcher (safe after QApplication)
    ThemeWatcher watcher(&app);
    app.installEventFilter(&watcher);

    // ✅ Defer ALL UI-touching startup work until event loop is active
    QTimer::singleShot(0, &app,
                       [&app]()
                       {
                           static QTranslator translator;
                           const bool loaded = loadAppTranslator(translator);

                           qDebug() << "Translator locale:" << normalizedLocale() << "loaded?" << loaded;
                           if (loaded)
                           { app.installTranslator(&translator); }
                           applyThemedAppIcon();
                       });
    // ✅ Create main window AFTER translator install is queued
    MainWindow window;
    window.show();
    return app.exec();
}

/**********************************************************************************************************************************
 *  End of main.cpp
 *********************************************************************************************************************************/