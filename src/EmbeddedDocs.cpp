/*!
 * ********************************************************************************************************************************
 * @file EmbeddedDocs.cpp
 * @brief Implementation of embedded documentation helpers.
 * @authors     Jeffrey Scott Flesher using AI: Copilot
 * @date        2026-03-31
 *
 * @details
 * Implements psd::docs utilities used to load locale-aware README markdown from Qt resources
 * and display it inside a modal Usage dialog.
 *
 * @namespace psd::docs
 *********************************************************************************************************************************/

#include "EmbeddedDocs.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QLocale>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace psd::docs
{
    /*!
     * ****************************************************************************************************************************
     * @brief Normalizes the system locale for documentation selection.
     *****************************************************************************************************************************/
    QString normalizedLocaleForDocs()
    {
        const QString systemLocale = QLocale::system().name();
        if (systemLocale == QStringLiteral("zh_CN")) { return QStringLiteral("zh_CN"); }
        return systemLocale.section('_', 0, 0);
    }

    /*!
     * ****************************************************************************************************************************
     * @brief Reads a UTF-8 text file from Qt resources.
     *****************************************************************************************************************************/
    bool readResourceText(const QString &path, QString &outText)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) { return false; }

        outText = QString::fromUtf8(file.readAll());
        return true;
    }

    /*!
     * ****************************************************************************************************************************
     * @brief Loads the most appropriate embedded README markdown.
     *****************************************************************************************************************************/
    QString loadReadmeMarkdown()
    {
        QString markdown;
        const QString locale = normalizedLocaleForDocs();

        if (readResourceText(QStringLiteral(":/README_%1.md").arg(locale), markdown)) { return markdown; }

        if (readResourceText(QStringLiteral(":/README.md"), markdown)) { return markdown; }

        return QObject::tr("Unable to load embedded README.md from application resources.");
    }

    /*!
     * ****************************************************************************************************************************
     * @brief Displays the Usage dialog containing embedded documentation.
     *****************************************************************************************************************************/
    void showUsageDialog(QWidget *parent)
    {
        const QString markdown = loadReadmeMarkdown();

        QDialog dialog(parent);
        dialog.setWindowTitle(QObject::tr("Usage"));
        dialog.resize(900, 650);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QTextBrowser *browser = new QTextBrowser(&dialog);
        browser->setOpenExternalLinks(true);
        browser->setMarkdown(markdown);

        layout->addWidget(browser);

        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);

        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::close);

        layout->addWidget(buttons);

        dialog.exec();
    }

} // namespace psd::docs

/*!
 * ********************************************************************************************************************************
 * End of EmbeddedDocs.cpp
 *********************************************************************************************************************************/
