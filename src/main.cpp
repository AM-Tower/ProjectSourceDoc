/*!
 * ********************************************************************************************************************************
 * @file main.cpp
 * @brief Application entry point for ProjectSourceDoc.
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-03-26
 * @details
 *  This file initializes the Qt application object, installs runtime event filters, applies a
 * theme-aware application icon, installs the application translator from embedded resources, and
 * launches the main UI.
 *********************************************************************************************************************************/

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QFile>
#include <QIcon>
#include <QLocale>
#include <QPalette>
#include <QTranslator>
#include <QWidget>
#include <QtGlobal>

#include "MainWindow.h"

/*!
 * ********************************************************************************************************************************
 *  @brief Returns true when the active application palette implies a dark theme.
 *
 *  This function evaluates the window background lightness from the current application palette and
 * treats lower lightness values as "dark mode". The heuristic is intentionally simple and mirrors
 * the logic used by theme-aware icon selection in the UI layer.
 *
 *  @return True when the application appears to be using a dark theme; otherwise false.
 *********************************************************************************************************************************/
static bool isDarkTheme()
{
    // Interpret lower background lightness as "dark theme".
    return qApp->palette().color(QPalette::Window).lightness() < 128;
}

/*!
 * ********************************************************************************************************************************
 *  @brief Applies the theme-aware application icon.
 *
 *  This function selects a light or dark SVG icon from Qt resources based on the current palette.
 * It sets the application icon and updates all existing top-level widgets to ensure the window icon refreshes.
 *********************************************************************************************************************************/
static void applyThemedAppIcon()
{
    // Choose icon theme folder based on the current palette.
    const QString theme = isDarkTheme() ? "dark" : "light";
    // Build the resource path for the application icon.
    const QString path = QString(":/icons/%1/app.svg").arg(theme);
    // Create the icon from resources and apply it globally.
    const QIcon icon(path);
    QApplication::setWindowIcon(icon);
    // Update all existing top-level widgets so their window icons update immediately.
    for (QWidget *w : QApplication::topLevelWidgets())
    {
        // Apply the icon to each existing top-level window.
        w->setWindowIcon(icon);
    }
    // Provide explicit diagnostics for resource existence and icon validity.
    qDebug() << "App icon path:" << path << "exists?" << QFile::exists(path) << "isNull?" << icon.isNull();
}

/*!
 * ********************************************************************************************************************************
 *  @class ThemeWatcher
 *  @brief Application-wide event filter that refreshes theme-aware UI resources.
 *
 *  Theme changes are typically communicated through palette, style, or theme events. This event
 * filter listens for those events and reapplies the theme-aware application icon so the icon stays
 * in sync with the system's appearance preferences.
 **********************************************************************************************************************************/
class ThemeWatcher final : public QObject
{
    public:
        /*!
         * ************************************************************************************************************************
         *  @brief Constructs the theme watcher.
         *
         *  @param parent Optional QObject parent for lifetime management.
         *************************************************************************************************************************/
        explicit ThemeWatcher(QObject *parent = nullptr) : QObject(parent)
        {
            // No additional initialization is required.
        }

    protected:
        /*!
         * ************************************************************************************************************************
         *  @brief Filters application events and refreshes themed resources when needed.
         *
         *  When the application palette, theme, or style changes, this function reapplies the
         * theme-aware application icon so the icon matches the active appearance.
         *
         *  @param obj The object receiving the event.
         *  @param ev  The event being dispatched.
         *  @return False to allow normal event processing to continue.
         *************************************************************************************************************************/
        bool eventFilter(QObject *obj, QEvent *ev) override
        {
            // React only to events that commonly indicate an appearance/theme transition.
            switch (ev->type())
            {
                case QEvent::ApplicationPaletteChange:
                case QEvent::ThemeChange:
                case QEvent::StyleChange:
                    // Re-apply the theme-aware icon when appearance changes.
                    applyThemedAppIcon();
                    break;
                default:
                    // Ignore all other events.
                    break;
            }

            // Always allow default processing to continue.
            return QObject::eventFilter(obj, ev);
        }
};

/*!
 * ********************************************************************************************************************************
 *  @brief Normalizes the system locale for translation selection.
 *
 *  This function maps the system locale to the translation naming scheme used by the project:
 *  - Special-case "zh_CN" for Simplified Chinese.
 *  - Otherwise, reduce to the language code (e.g. "en_US" -> "en").
 *
 *  @return The normalized locale key used in translation filenames.
 *********************************************************************************************************************************/
static QString normalizedLocale()
{
    // Acquire the system locale in "ll_CC" form (e.g. "en_US", "zh_CN").
    const QString systemLocale = QLocale::system().name();

    // Preserve "zh_CN" exactly, otherwise return language only.
    if (systemLocale == "zh_CN")
    {
        return "zh_CN";
    }

    // Reduce "en_US" -> "en", "fr_FR" -> "fr", etc.
    return systemLocale.section('_', 0, 0);
}

/*!
 * ********************************************************************************************************************************
 *  @brief Attempts to load the application translator from embedded resources.
 *
 *  The build embeds generated .qm translation files into the Qt resource system under the "/i18n" prefix.
 *  This function tries the normalized locale first, then falls back to English.
 *
 *  @param translator Translator instance to load into.
 *  @return True when a translation was loaded successfully; otherwise false.
 *********************************************************************************************************************************/
static bool loadAppTranslator(QTranslator &translator)
{
    // Determine the normalized locale key used by translation filenames.
    const QString loc = normalizedLocale();

    // Build the preferred resource path for the requested locale.
    const QString preferred = QString(":/i18n/ProjectSourceDoc_%1.qm").arg(loc);

    // Attempt to load the preferred locale translation first.
    if (translator.load(preferred))
    {
        return true;
    }

    // Fall back to English if the preferred locale is unavailable.
    const QString fallback = QString(":/i18n/ProjectSourceDoc_en.qm");
    if (translator.load(fallback))
    {
        return true;
    }

    // No translations could be loaded.
    return false;
}

/*!
 * ********************************************************************************************************************************
 *  @brief Program entry point.
 *
 *  Initializes QApplication, installs a theme watcher, applies a theme-aware icon, installs
 * translations, and launches the main window.
 *
 *  @param argc Command line argument count.
 *  @param argv Command line argument vector.
 *  @return Process exit code from the Qt event loop.
 *********************************************************************************************************************************/
int main(int argc, char *argv[])
{
    #ifdef __MINGW32__
    // Force Qt 6 to use OpenGL instead of D3D12 on MinGW for improved compatibility.
    qputenv("QT_DEFAULT_RHI", "opengl");
    qDebug() << "QT_DEFAULT_RHI =" << qEnvironmentVariable("QT_DEFAULT_RHI");

    // Ensure the Windows platform plugin is selected on MinGW.
    qputenv("QT_QPA_PLATFORM", "windows");
    qDebug() << "QT_QPA_PLATFORM =" << qEnvironmentVariable("QT_QPA_PLATFORM");
    #endif
    // Create the application object before any GUI objects are constructed.
    QApplication app(argc, argv);
    // Set application identity metadata.
    QApplication::setApplicationName("ProjectSourceDoc");
    QApplication::setOrganizationName("ProjectSourceDoc");
    QApplication::setApplicationVersion("0.1.0");
    // Install the theme watcher before creating any windows to catch early theme-related events.
    ThemeWatcher watcher(&app);
    app.installEventFilter(&watcher);
    // Apply the initial theme-aware icon before showing windows.
    applyThemedAppIcon();
    // Install the application translator from embedded resources so tr() resolves correctly.
    QTranslator translator;
    const bool translatorLoaded = loadAppTranslator(translator);
    // Log translator load results for debugging.
    qDebug() << "Translator locale:" << normalizedLocale() << "loaded?" << translatorLoaded;
    // Install the translator only when a translation was successfully loaded.
    if (translatorLoaded) { app.installTranslator(&translator); }
    // Create the main window after translator installation so UI strings are translated at
    // construction.
    MainWindow window;
    window.show();
    // Enter the Qt event loop.
    return app.exec();
}

/**********************************************************************************************************************************
 *  End of main.cpp
 *********************************************************************************************************************************/
