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

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QTabWidget>

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
        case QEvent::ApplicationPaletteChange:
        case QEvent::PaletteChange:
        case QEvent::ThemeChange:
        case QEvent::StyleChange: refreshThemeUi(); break;
        default:  break;
    }
}

/*!
 * ********************************************************************************************************************************
 * End of MainWindowTheme.cpp
 *********************************************************************************************************************************/
