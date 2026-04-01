/*!
 * ********************************************************************************************************************************
 * @file        MainWindowTheme.cpp
 * @brief       Implements theme-aware icon refresh for ProjectSource MainWindow.
 * @details     Refreshes icons when application palette/theme changes.
 *
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-03-26
 *********************************************************************************************************************************/
#include "MainWindow.h"

#include <QApplication>
#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QTabWidget>
#include <QStyle>
#include <QStyleFactory>
#include <QSettings>
#include <QStatusBar>
#include <QColor>

/*!
 * ********************************************************************************************************************************
 * @brief Refreshes UI elements that depend on the current palette (icons in menus/toolbars/tabs).
 * @details Updates icons in-place without rebuilding toolbars. Rebuilds MRU menu to refresh
 * per-item icons.
 *********************************************************************************************************************************/
void MainWindow::refreshThemeUi()
{
    if (m_openAction)          { m_openAction->setIcon(themedIcon(QStringLiteral("doc"))); }
    if (m_projectSourceAction) { m_projectSourceAction->setIcon(themedIcon(QStringLiteral("update"))); }
    if (m_exitAction)          { m_exitAction->setIcon(themedIcon(QStringLiteral("exit"))); }
    if (m_usageAction)         { m_usageAction->setIcon(themedIcon(QStringLiteral("help"))); }
    if (m_aboutAction)         { m_aboutAction->setIcon(themedIcon(QStringLiteral("help"))); }
    rebuildRecentProjectsMenu();
    if (m_tabs && m_tabs->count() > 0) { m_tabs->setTabIcon(0, themedIcon(QStringLiteral("log"))); }
    if (m_tabs && m_tabs->count() > 1) { m_tabs->setTabIcon(1, themedIcon(QStringLiteral("settings"))); }
}

/*!
 * ********************************************************************************************************************************
 * @brief Handles palette/theme changes at runtime.
 * @param event Change event.
 *********************************************************************************************************************************/
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (!event) return;

    switch (event->type())
    {
    case QEvent::LanguageChange:
    {
        // Retranslate UI text without rebuilding structure
        if (m_openAction)        m_openAction->setText(tr("&Open…"));
        if (m_projectSourceAction)
            m_projectSourceAction->setText(tr("Create Project‑Source.txt"));
        if (m_exitAction)        m_exitAction->setText(tr("E&xit"));
        if (m_usageAction)       m_usageAction->setText(tr("Usage"));
        if (m_aboutAction)       m_aboutAction->setText(tr("About"));

               // Menus need title updates
        if (m_recentMenu)        m_recentMenu->setTitle(tr("Open &Recent"));

               // Status / tab text if needed later
               // (icons are already handled elsewhere)
        break;
    }

    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::ThemeChange:
    case QEvent::StyleChange:
        refreshThemeUi();
        break;

    default:
        break;
    }
}

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
void MainWindow::applySystemTheme()
{
    qApp->setStyleSheet(QString());
    // Restore original platform style if we previously forced Fusion.
    if (!m_platformStyleName.isEmpty())
    {
        if (QStyle *s = QStyleFactory::create(m_platformStyleName)) qApp->setStyle(s);
    }
    qApp->setPalette(qApp->style()->standardPalette());
}

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
void MainWindow::applyLightTheme()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, QColor(245, 245, 245));
    p.setColor(QPalette::WindowText, Qt::black);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::Text, Qt::black);
    p.setColor(QPalette::Button, QColor(240, 240, 240));
    p.setColor(QPalette::ButtonText, Qt::black);

    qApp->setPalette(p);
}

/*!
 * ********************************************************************************************************************************
 * @brief Apply a forced dark theme.
 *
 * Applies a dark application palette designed for low-light environments.
 * This overrides the system theme until changed by the user.
 *
 * Theme-aware icons and widgets are refreshed automatically through palette
 * change events.
 ***********************************************************************************************************************************/
void MainWindow::applyDarkTheme()
{
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window, QColor(32, 32, 32));
    p.setColor(QPalette::WindowText, Qt::white);
    p.setColor(QPalette::Base, QColor(24, 24, 24));
    p.setColor(QPalette::Text, Qt::white);
    p.setColor(QPalette::Button, QColor(45, 45, 45));
    p.setColor(QPalette::ButtonText, Qt::white);

    qApp->setPalette(p);
}

/*!
 * ********************************************************************************************************************************
 * @brief Save the selected theme mode to persistent settings.
 * @param mode Selected theme mode.
 ***********************************************************************************************************************************/
void MainWindow::saveThemeMode(ThemeMode mode)
{
    QSettings settings;
    settings.setValue(kSettingsThemeMode, static_cast<int>(mode));
}

/*!
 * ********************************************************************************************************************************
 * End of MainWindowTheme.cpp
 *********************************************************************************************************************************/
