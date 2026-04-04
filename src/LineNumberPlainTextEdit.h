/*!
 * ********************************************************************************************************************************
 * @file        LineNumberPlainTextEdit.h
 * @brief       Declares a QPlainTextEdit subclass with a line number margin.
 * @details     Provides an integrated line number area widget, width calculation, painting hooks,
 *              and viewport margin management. Used in editors requiring visible line numbers.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-04
 *********************************************************************************************************************************/

#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class LineNumberArea;

/*!
 * ****************************************************************************************************************************
 * @brief   A QPlainTextEdit subclass that displays line numbers.
 * @details Manages a dedicated line number area widget, handles painting, geometry updates,
 *          and viewport margin adjustments to keep line numbers aligned with text blocks.
 *****************************************************************************************************************************/
class LineNumberPlainTextEdit : public QPlainTextEdit
{
        Q_OBJECT

    public:
        /*!
         * ************************************************************************************************************************
         * @brief   Construct the LineNumberPlainTextEdit.
         * @param   parent Optional parent widget.
         *************************************************************************************************************************/
        explicit LineNumberPlainTextEdit(QWidget *parent = nullptr);

        /*!
         * ************************************************************************************************************************
         * @brief   Calculate the width required for the line number area.
         * @return  Width in pixels based on the number of digits.
         *************************************************************************************************************************/
        int lineNumberAreaWidth() const;

        /*!
         * ************************************************************************************************************************
         * @brief   Paint the line number area.
         * @param   event Paint event.
         *************************************************************************************************************************/
        void lineNumberAreaPaintEvent(QPaintEvent *event);

    protected:
        /*!
         * ************************************************************************************************************************
         * @brief   Handle resize events and reposition the line number area.
         * @param   event Resize event.
         *************************************************************************************************************************/
        void resizeEvent(QResizeEvent *event) override;

    private slots:
        /*!
         * ************************************************************************************************************************
         * @brief   Update viewport margins based on the line number area width.
         *************************************************************************************************************************/
        void updateLineNumberAreaWidth();

        /*!
         * ************************************************************************************************************************
         * @brief   Update the line number area when scrolling or redrawing.
         * @param   rect Updated rectangle.
         * @param   dy   Vertical scroll offset.
         *************************************************************************************************************************/
        void updateLineNumberArea(const QRect &rect, int dy);

    private:
        LineNumberArea *m_lineNumberArea; //!< Line number area widget.
};

/*!
 * ********************************************************************************************************************************
 * @brief End of LineNumberPlainTextEdit.h
 *********************************************************************************************************************************/
