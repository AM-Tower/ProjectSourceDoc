/*!
 * ********************************************************************************************************************************
 * @file        MainWindowTheme.cpp
 * @brief       Implements theme persistence, theme application, and theme-aware UI refresh.
 * @details     Centralizes theme menu creation, save/load of ThemeMode, palette application, and icon refresh
 *              for menus/toolbars/tabs based on the active palette. Designed to be reusable across apps that
 *              provide a Tools → Theme submenu.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-05
 *********************************************************************************************************************************/
#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QPalette>
#include <QSettings>
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleFactory>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

namespace
{
    static constexpr const char *kSettingsThemeMode = "ui/themeMode";
}

/*!
 * ****************************************************************************************************************************
 * @brief   Creates the Tools → Theme submenu.
 * @details Creates System, Light, and Dark actions and wires them through applyThemeMode().
 * @param   toolsMenu Tools menu that will own the Theme submenu.
 *****************************************************************************************************************************/
void MainWindow::createThemeMenu(QMenu *toolsMenu)
{
    if (!toolsMenu) return;

    m_themeMenu = toolsMenu->addMenu(tr("Theme"));

    m_themeGroup = new QActionGroup(this);
    m_themeGroup->setExclusive(true);

    m_themeSystemAction = m_themeMenu->addAction(tr("System"));
    m_themeLightAction = m_themeMenu->addAction(tr("Light"));
    m_themeDarkAction = m_themeMenu->addAction(tr("Dark"));

    m_themeSystemAction->setCheckable(true);
    m_themeLightAction->setCheckable(true);
    m_themeDarkAction->setCheckable(true);

    m_themeGroup->addAction(m_themeSystemAction);
    m_themeGroup->addAction(m_themeLightAction);
    m_themeGroup->addAction(m_themeDarkAction);

    connect(m_themeSystemAction, &QAction::triggered, this, [this]() { applyThemeMode(ThemeMode::System, true); });
    connect(m_themeLightAction, &QAction::triggered, this, [this]() { applyThemeMode(ThemeMode::Light, true); });
    connect(m_themeDarkAction, &QAction::triggered, this, [this]() { applyThemeMode(ThemeMode::Dark, true); });
}

/*!
 * ****************************************************************************************************************************
 * @brief   Load the persisted theme mode from QSettings.
 * @details Validates stored integer and defaults to System when invalid.
 * @return  Persisted ThemeMode.
 *****************************************************************************************************************************/
ThemeMode MainWindow::loadThemeMode() const
{
    QSettings settings;
    const int value = settings.value(kSettingsThemeMode, static_cast<int>(ThemeMode::System)).toInt();

    switch (static_cast<ThemeMode>(value))
    {
    case ThemeMode::System: return ThemeMode::System;
    case ThemeMode::Light:  return ThemeMode::Light;
    case ThemeMode::Dark:   return ThemeMode::Dark;
    }

    return ThemeMode::System;
}

/*!
 * ****************************************************************************************************************************
 * @brief   Save the selected theme mode to persistent settings.
 * @param   mode Selected theme mode.
 *****************************************************************************************************************************/
void MainWindow::saveThemeMode(ThemeMode mode)
{
    QSettings settings;
    settings.setValue(kSettingsThemeMode, static_cast<int>(mode));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Update the Theme menu checkmarks to match the specified theme mode.
 * @details Blocks signals to prevent recursive triggers while updating checks.
 * @param   mode Theme mode that should appear selected in the menu.
 *****************************************************************************************************************************/
void MainWindow::updateThemeMenuChecks(ThemeMode mode)
{
    if (!m_themeSystemAction || !m_themeLightAction || !m_themeDarkAction) return;

    const QSignalBlocker b1(m_themeSystemAction);
    const QSignalBlocker b2(m_themeLightAction);
    const QSignalBlocker b3(m_themeDarkAction);

    m_themeSystemAction->setChecked(mode == ThemeMode::System);
    m_themeLightAction->setChecked(mode == ThemeMode::Light);
    m_themeDarkAction->setChecked(mode == ThemeMode::Dark);
}

/*!
 * ****************************************************************************************************************************
 * @brief   Apply a theme mode and synchronize UI.
 * @details Applies palette/style, optionally persists, updates checkmarks, and refreshes themed icons.
 * @param   mode Theme mode to apply.
 * @param   persist True to save the mode to QSettings.
 *****************************************************************************************************************************/
void MainWindow::applyThemeMode(ThemeMode mode, bool persist)
{
    if (mode == ThemeMode::Light) applyLightTheme();
    else if (mode == ThemeMode::Dark) applyDarkTheme();
    else applySystemTheme();

    if (persist) saveThemeMode(mode);

    updateThemeMenuChecks(mode);
    refreshThemeUi();
}

/*!
 * ****************************************************************************************************************************
 * @brief   Restore the persisted theme mode from settings.
 * @details Applies the stored theme at startup and syncs the theme menu checkmarks.
 *****************************************************************************************************************************/
void MainWindow::restoreThemeMode()
{
    applyThemeMode(loadThemeMode(), false);
}

/*!
 * ****************************************************************************************************************************
 * @brief   Retranslate only the Theme menu UI.
 * @details Updates Theme menu title and Theme action texts.
 *****************************************************************************************************************************/
void MainWindow::retranslateThemeMenu()
{
    if (m_themeMenu) m_themeMenu->setTitle(tr("Theme"));
    if (m_themeSystemAction) m_themeSystemAction->setText(tr("System"));
    if (m_themeLightAction) m_themeLightAction->setText(tr("Light"));
    if (m_themeDarkAction) m_themeDarkAction->setText(tr("Dark"));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Handle palette/style/theme events to keep icons in sync.
 * @details Refreshes theme-dependent icons and keeps theme menu checkmarks consistent with persisted mode.
 * @param   event Change event.
 *****************************************************************************************************************************/
void MainWindow::handleThemeChangeEvent(QEvent *event)
{
    if (!event) return;

    switch (event->type())
    {
    case QEvent::ApplicationPaletteChange:
    case QEvent::PaletteChange:
    case QEvent::ThemeChange:
    case QEvent::StyleChange:
        updateThemeMenuChecks(loadThemeMode());
        refreshThemeUi();
        break;

    default:
        break;
    }
}

/*!
 * ****************************************************************************************************************************
 * @brief   Handles language, palette, theme, and style changes.
 * @details Retranslates the Theme menu on language changes and refreshes theme UI on palette/style changes.
 * @param   event Change event.
 *****************************************************************************************************************************/
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if (!event) return;

    if (event->type() == QEvent::LanguageChange)
    {
        retranslateThemeMenu();
        return;
    }

    handleThemeChangeEvent(event);
}

/*!
 * ****************************************************************************************************************************
 * @brief   Refreshes UI elements that depend on the current palette.
 * @details Updates icons in menus, toolbars, and tab widgets using theme-aware icons.
 *****************************************************************************************************************************/
void MainWindow::refreshThemeUi()
{
    if (menuBar())
    {
        const QList<QAction *> topActions = menuBar()->actions();
        for (QAction *a : std::as_const(topActions))
        {
            refreshActionIcon(a);

            if (QMenu *menu = a->menu())
            {
                const QList<QAction *> actions = menu->actions();
                for (QAction *sub : std::as_const(actions))
                {
                    refreshActionIcon(sub);
                    if (QMenu *subMenu = sub->menu())
                    {
                        const QList<QAction *> subActions = subMenu->actions();
                        for (QAction *deep : std::as_const(subActions))
                        {
                            refreshActionIcon(deep);
                        }
                    }
                }
            }
        }
    }

    const QList<QToolBar *> bars = findChildren<QToolBar *>();
    for (QToolBar *bar : std::as_const(bars))
    {
        const QList<QAction *> actions = bar->actions();
        for (QAction *a : std::as_const(actions)) refreshActionIcon(a);

        const QList<QToolButton *> buttons = bar->findChildren<QToolButton *>();
        for (QToolButton *b : std::as_const(buttons))
        {
            const QString name = b->property("iconName").toString();
            if (!name.isEmpty()) b->setIcon(themedIcon(name));
        }
    }

    const QList<QTabWidget *> tabs = findChildren<QTabWidget *>();
    for (QTabWidget *tw : std::as_const(tabs))
    {
        const QStringList names = tw->property("tabIconNames").toStringList();
        const int count = qMin(names.size(), tw->count());
        for (int i = 0; i < count; ++i)
        {
            tw->setTabIcon(i, themedIcon(names.at(i)));
        }
    }
}

/*!
 * ****************************************************************************************************************************
 * @brief   Refresh a QAction icon if it declares an "iconName" property.
 * @param   action Action whose icon should be refreshed.
 *****************************************************************************************************************************/
void MainWindow::refreshActionIcon(QAction *action)
{
    if (!action) return;

    const QString name = action->property("iconName").toString();
    if (name.isEmpty()) return;

    action->setIcon(themedIcon(name));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Apply the system (default) theme.
 * @details Clears stylesheet overrides and restores the platform default style (if captured) and palette.
 *****************************************************************************************************************************/
void MainWindow::applySystemTheme()
{
    qApp->setStyleSheet(QString());

    if (!m_platformStyleName.isEmpty())
    {
        if (QStyle *s = QStyleFactory::create(m_platformStyleName)) qApp->setStyle(s);
    }

    qApp->setPalette(qApp->style()->standardPalette());
}

/*!
 * ****************************************************************************************************************************
 * @brief   Apply a forced light theme.
 * @details Applies a light palette and uses Fusion style for consistency.
 *****************************************************************************************************************************/
void MainWindow::applyLightTheme()
{
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

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
 * ****************************************************************************************************************************
 * @brief   Apply a forced dark theme.
 * @details Applies a dark palette and uses Fusion style for consistency.
 *****************************************************************************************************************************/
void MainWindow::applyDarkTheme()
{
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

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
 * ****************************************************************************************************************************
 * @brief   Returns a theme-aware icon from application resources.
 * @details Selects :/icons/light/<name>.svg or :/icons/dark/<name>.svg based on current palette.
 * @param   baseName Base icon name without extension.
 * @return  Theme-aware icon.
 *****************************************************************************************************************************/
QIcon MainWindow::themedIcon(const QString &baseName) const
{
    const bool dark = qApp->palette().color(QPalette::Window).lightness() < 128;
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    return QIcon(QStringLiteral(":/icons/%1/%2.svg").arg(theme, baseName));
}

/*!
 * ********************************************************************************************************************************
 * @brief End of MainWindowTheme.cpp
 *********************************************************************************************************************************/
