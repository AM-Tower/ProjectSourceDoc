/*!
 * ********************************************************************************************************************************
 * @file        AboutDialog.cpp
 * @brief       About dialog implementation.
 * @details     Displays application information in a modal dialog without UI files.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-02
 *********************************************************************************************************************************/
#include "AboutDialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

/*!
 * ****************************************************************************************************************************
 * @brief   Construct AboutDialog.
 * @details Builds a simple modal About dialog using standard Qt widgets.
 * @param   parent Optional parent widget.
 *****************************************************************************************************************************/
AboutDialog::AboutDialog(QWidget *parent)
: QDialog(parent)
{
    setWindowTitle(tr("About"));
    setModal(true);
    setMinimumWidth(420);

    const QString appName = QApplication::applicationName();
    const QString appVersion = QApplication::applicationVersion();

    QLabel *titleLabel = new QLabel(QStringLiteral("<b>%1</b>").arg(appName), this);
    QLabel *versionLabel = new QLabel(tr("Version %1").arg(appVersion), this);
    QLabel *descriptionLabel =
        new QLabel(
            tr("Generate Project‑Source.txt from a project tree.<br><br>"
               "This tool replaces legacy CMake PackSource scripts<br>"
               "with a native C++ / Qt implementation.<br><br>"
               "© 2026 Jeffrey Scott Flesher"),
            this);

    titleLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setAlignment(Qt::AlignCenter);
    descriptionLabel->setWordWrap(true);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addSpacing(12);
    layout->addWidget(descriptionLabel);
    layout->addStretch();
    layout->addWidget(buttons);

    setLayout(layout);
}

/*!
 * ********************************************************************************************************************************
 * @brief End of AboutDialog.cpp
 *********************************************************************************************************************************/
