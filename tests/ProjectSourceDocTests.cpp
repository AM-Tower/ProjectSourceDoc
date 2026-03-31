/*!
 * ********************************************************************************************************************************
 * @file         ProjectSourceDocTests.cpp
 * @brief        Unit tests for critical ProjectSourceDoc utilities and helpers.
 * @details      Uses Qt Test to validate deterministic logic, filesystem behavior,
 *               string normalization, and UI‑safe boundary conditions.
 *
 *               These tests intentionally avoid full UI instantiation and focus
 *               on pure logic, filesystem effects, and message routing.
 *
 * @author       Jeffrey Scott Flesher
 * @date         2026-03-31
 *********************************************************************************************************************************/

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMainWindow>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QtTest/QtTest>

#include "AppLogUtils.h"
#include "MainWindow.h"
#include "ProjectSourceExporter.h"
#include "StatusBarQueue.h"

/*!
 * @brief Read the entire contents of a text file.
 *
 * @param path Absolute or relative path to the file.
 * @return File contents as UTF‑8 text, or an empty QString on failure.
 */
static QString readAllTextFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
    return QString::fromUtf8(f.readAll());
}

/*!
 * @brief Write text to a file, truncating any existing content.
 *
 * @param path Absolute or relative path to the file.
 * @param text UTF‑8 text to write.
 * @return true if the write succeeded, false otherwise.
 */
static bool writeAllTextFile(const QString &path, const QString &text)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;
    f.write(text.toUtf8());
    return true;
}

/*!
 * ********************************************************************************************************************************
 * @class        ProjectSourceDocTests
 * @brief        QtTest suite for ProjectSourceDoc.
 *
 * @details
 * This test suite validates:
 *  - Log file lifecycle behavior
 *  - String normalization and hashing helpers
 *  - .gitignore directory‑rule parsing semantics
 *  - Project source generation boundary behavior
 *  - Tree generation ordering guarantees
 *  - Status bar message queuing and persistence
 *
 * All tests are designed to be deterministic and fast.
 *********************************************************************************************************************************/
class ProjectSourceDocTests final : public QObject
{
        Q_OBJECT

    private slots:

        /* ------------------------------------------------------------------------------------------------
         * AppLogUtils
         * ------------------------------------------------------------------------------------------------
         */

        /*!
         * @brief Verify that startup log cleanup truncates existing log files.
         *
         * @details
         * Ensures both message and error log files are cleared when
         * clearLogFilesAtStartup() is invoked.
         */
        void clearLogFilesAtStartup_truncatesFiles()
        {
            QTemporaryDir temp;
            QVERIFY2(temp.isValid(), "Failed to create temp directory.");

            const QString oldCwd = QDir::currentPath();
            QVERIFY(QDir::setCurrent(temp.path()));

            const QString msgPath = QDir(temp.path()).filePath(QStringLiteral("message-queue.txt"));
            const QString errPath = QDir(temp.path()).filePath(QStringLiteral("error-log.txt"));

            QVERIFY(writeAllTextFile(msgPath, QStringLiteral("hello")));
            QVERIFY(writeAllTextFile(errPath, QStringLiteral("world")));

            psd::log::clearLogFilesAtStartup();

            QCOMPARE(readAllTextFile(msgPath), QString());
            QCOMPARE(readAllTextFile(errPath), QString());

            QVERIFY(QDir::setCurrent(oldCwd));
        }

        /*!
         * @brief Verify that logAppend() appends text without formatting side effects.
         */
        void logAppend_appendsPlainText()
        {
            QTextEdit edit;
            edit.setPlainText(QStringLiteral("A"));

            psd::log::logAppend(&edit, QStringLiteral("B"));
            QCOMPARE(edit.toPlainText(), QStringLiteral("AB"));
        }

        /*!
         * @brief Verify bash‑safe escaping of single quotes.
         *
         * @details
         * Ensures single quotes are escaped in a way compatible with
         * single‑quoted shell strings.
         */
        void bashEscapeSingleQuoted_escapesSingleQuotes()
        {
            const QString in = QStringLiteral("abc'def");
            const QString out = psd::log::bashEscapeSingleQuoted(in);

            // Expected bash-safe form: abc'\''def
            QCOMPARE(out, QStringLiteral("abc'\\''def"));
        }

        /* ------------------------------------------------------------------------------------------------
         * MainWindow helper logic
         * ------------------------------------------------------------------------------------------------
         */

        /*!
         * @brief Verify extension parsing and normalization.
         *
         * @details
         * Confirms separators, dots, case, and duplicates are handled correctly.
         */
        void parseExtensionsText_normalizesSeparatorsDotsCaseAndDedup()
        {
            const QString raw = QStringLiteral("  .CPP, h ; md  cpp  ,  .h ");
            const QStringList got = MainWindow::parseExtensionsText(raw);

            QCOMPARE(got, (QStringList{QStringLiteral("cpp"), QStringLiteral("h"), QStringLiteral("md")}));
        }

        /*!
         * @brief Verify stable MD5 generation for project paths.
         */
        void projectIdForPath_isStableMd5()
        {
            // MD5("abc") = 900150983cd24fb0d6963f7d28e17f72
            const QString id = MainWindow::projectIdForPath(QStringLiteral("abc"));
            QCOMPARE(id, QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
        }

        /*!
         * @brief Verify .gitignore directory rule parsing.
         *
         * @details
         * Confirms that:
         *  - Only rules ending in '/' are treated as directory exclusions
         *  - Negations are ignored
         *  - Dots in directory names are allowed
         *  - File‑only rules are excluded
         */
        void excludeDirsFromGitignore_parsesDirectoryRulesOnly()
        {
            QTemporaryDir temp;
            QVERIFY2(temp.isValid(), "Failed to create temp directory.");

            const QString root = temp.path();
            const QString gitignorePath = QDir(root).filePath(QStringLiteral(".gitignore"));

            const QString content = "# comment\n"
                                    "\n"
                                    "build/\n"
                                    "!keep/\n"
                                    "file.txt/\n"
                                    "cache/*/\n"
                                    ".git/\n"
                                    "*.log/\n"
                                    "dist\n";

            QVERIFY(writeAllTextFile(gitignorePath, content));

            const QStringList result = MainWindow::excludeDirsFromGitignore(root);

            // ✅ Directory rules
            QVERIFY(result.contains(QStringLiteral("build")));
            QVERIFY(result.contains(QStringLiteral("cache")));
            QVERIFY(result.contains(QStringLiteral(".git")));
            QVERIFY(result.contains(QStringLiteral("*.log")));
            QVERIFY(result.contains(QStringLiteral("file.txt")));

            // ❌ Non-directory rules
            QVERIFY(!result.contains(QStringLiteral("keep")));
            QVERIFY(!result.contains(QStringLiteral("dist")));
        }

        /* ------------------------------------------------------------------------------------------------
         * ProjectSourceExporter
         * ------------------------------------------------------------------------------------------------
         */

        /*!
         * @brief Verify exporter returns empty output for invalid roots.
         */
        void generateAndReturnPath_returnsEmptyOnInvalidRoot()
        {
            const QString out1 = ProjectSourceExporter::generateAndReturnPath(nullptr, nullptr, QString(), QStringList{}, QStringList{}, nullptr);
            QVERIFY(out1.isEmpty());

            const QString out2 = ProjectSourceExporter::generateAndReturnPath(nullptr, nullptr, QStringLiteral("/path/does/not/exist"), QStringList{}, QStringList{}, nullptr);
            QVERIFY(out2.isEmpty());
        }

        /*!
         * @brief Verify directory tree generation orders directories before files.
         */
        void createTree_writesTreeAndSortsDirsBeforeFiles()
        {
            QTemporaryDir temp;
            QVERIFY2(temp.isValid(), "Failed to create temp directory.");

            const QString root = temp.path();

            QVERIFY(QDir(root).mkpath(QStringLiteral("adir")));
            QVERIFY(writeAllTextFile(QDir(root).filePath(QStringLiteral("zzz.txt")), QStringLiteral("x")));

            QString log;
            const QString outPath = ProjectSourceExporter::createTree(nullptr, nullptr, root, QStringList{QStringLiteral("cpp")}, QStringList{}, &log);

            QVERIFY(!outPath.isEmpty());
            QVERIFY(QFileInfo::exists(outPath));

            const QString treeText = readAllTextFile(outPath);

            const int posDir = treeText.indexOf(QStringLiteral("adir"));
            const int posFile = treeText.indexOf(QStringLiteral("zzz.txt"));

            QVERIFY(posDir >= 0);
            QVERIFY(posFile >= 0);
            QVERIFY2(posDir < posFile, "Expected directories to be listed before files.");
        }

        /* ------------------------------------------------------------------------------------------------
         * StatusBarQueue
         * ------------------------------------------------------------------------------------------------
         */

        /*!
         * @brief Verify status bar messages are displayed and persisted.
         */
        void enqueue_setsStatusBarMessage_andWritesMessageLog()
        {
            QTemporaryDir temp;
            QVERIFY2(temp.isValid(), "Failed to create temp directory.");

            const QString root = temp.path();
            QVERIFY(writeAllTextFile(QDir(root).filePath(QStringLiteral("CMakeLists.txt")), QStringLiteral("cmake_minimum_required(VERSION 3.25)\n")));

            const QString oldCwd = QDir::currentPath();
            QVERIFY(QDir::setCurrent(root));

            QMainWindow host;
            QStatusBar sb;
            host.setStatusBar(&sb);

            StatusBarQueue queue(&sb, &host);
            queue.enqueue(QStringLiteral("Hello"), 0);

            QTRY_COMPARE(sb.currentMessage(), QStringLiteral("Hello"));

            const QString logPath = QDir(root).filePath(QStringLiteral("message-queue.txt"));
            QVERIFY(QFileInfo::exists(logPath));
            QVERIFY(readAllTextFile(logPath).contains(QStringLiteral("Hello")));

            QVERIFY(QDir::setCurrent(oldCwd));
        }
};

QTEST_MAIN(ProjectSourceDocTests)
#include "ProjectSourceDocTests.moc"

/*!
 * ********************************************************************************************************************************
 * @file         ProjectSourceDocTests.cpp
 * @brief        End of ProjectSourceDocTests.cpp
 *********************************************************************************************************************************/
