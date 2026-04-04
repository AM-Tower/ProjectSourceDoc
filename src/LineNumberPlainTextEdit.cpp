/*!
 * ********************************************************************************************************************************
 * @file        LineNumberPlainTextEdit.cpp
 * @brief       Implements a QPlainTextEdit subclass with a line number margin.
 * @details     Provides a custom line number area widget, width calculation, painting logic,
 *              and viewport margin updates. This component is typically used in code editors
 *              or text editors requiring visible line numbers.
 * @authors     Jeffrey Scott Flesher with the help of AI: Copilot
 * @date        2026-04-04
 *********************************************************************************************************************************/

#include "LineNumberPlainTextEdit.h"
#include <QPainter>
#include <QTextBlock>

/*!
 * ****************************************************************************************************************************
 * @brief   Internal helper widget used to render the line number area.
 * @details Delegates all painting to LineNumberPlainTextEdit::lineNumberAreaPaintEvent().
 *****************************************************************************************************************************/
class LineNumberArea : public QWidget
{
    public:
        /*!
         * ************************************************************************************************************************
         * @brief   Construct the line number area widget.
         * @param   editor Pointer to the owning LineNumberPlainTextEdit.
         *************************************************************************************************************************/
        explicit LineNumberArea(LineNumberPlainTextEdit *editor) : QWidget(editor), m_editor(editor)
        {
        }

        /*!
         * ************************************************************************************************************************
         * @brief   Provide a recommended size for the line number area.
         * @return  Width based on digit count; height is flexible.
         *************************************************************************************************************************/
        QSize sizeHint() const override
        {
            return QSize(m_editor->lineNumberAreaWidth(), 0);
        }

    protected:
        /*!
         * ************************************************************************************************************************
         * @brief   Paint event handler for the line number area.
         * @param   event Paint event.
         *************************************************************************************************************************/
        void paintEvent(QPaintEvent *event) override
        {
            m_editor->lineNumberAreaPaintEvent(event);
        }

    private:
        LineNumberPlainTextEdit *m_editor; //!< Owning editor instance.
};

/*!
 * ****************************************************************************************************************************
 * @brief   Construct the LineNumberPlainTextEdit.
 * @param   parent Optional parent widget.
 *****************************************************************************************************************************/
LineNumberPlainTextEdit::LineNumberPlainTextEdit(QWidget *parent) : QPlainTextEdit(parent), m_lineNumberArea(new LineNumberArea(this))
{
    connect(this, &QPlainTextEdit::blockCountChanged, this, &LineNumberPlainTextEdit::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &LineNumberPlainTextEdit::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &LineNumberPlainTextEdit::updateLineNumberAreaWidth);

    updateLineNumberAreaWidth();
}

/*!
 * ****************************************************************************************************************************
 * @brief   Calculate the width required for the line number area.
 * @return  Width in pixels based on the number of digits plus spacing.
 *****************************************************************************************************************************/
int LineNumberPlainTextEdit::lineNumberAreaWidth() const
{
    const int digits = QString::number(blockCount()).length();
    const int digitWidth = fontMetrics().horizontalAdvance(QLatin1Char('9'));
    const int spaceWidth = fontMetrics().horizontalAdvance(QLatin1Char(' '));

    return 3 + (digitWidth * digits) + spaceWidth + (spaceWidth * digits);
}

/*!
 * ****************************************************************************************************************************
 * @brief   Update viewport margins based on the line number area width.
 *****************************************************************************************************************************/
void LineNumberPlainTextEdit::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

/*!
 * ****************************************************************************************************************************
 * @brief   Update the line number area when scrolling or redrawing.
 * @param   rect Updated rectangle.
 * @param   dy   Vertical scroll offset.
 *****************************************************************************************************************************/
void LineNumberPlainTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy) m_lineNumberArea->scroll(0, dy);
    else m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth();
}

/*!
 * ****************************************************************************************************************************
 * @brief   Handle resize events and reposition the line number area.
 * @param   event Resize event.
 *****************************************************************************************************************************/
void LineNumberPlainTextEdit::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

/*!
 * ****************************************************************************************************************************
 * @brief   Paint the line numbers for visible text blocks.
 * @param   event Paint event.
 *****************************************************************************************************************************/
void LineNumberPlainTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().window());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();

    int top = static_cast<int>(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + static_cast<int>(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom())
    {
        if (block.isVisible() && bottom >= event->rect().top())
        {
            const QString number = QString::number(blockNumber + 1) + QLatin1Char(' ');
            painter.setPen(palette().text().color());
            painter.drawText(0, top, m_lineNumberArea->width() - 4, fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

/*!
 * ********************************************************************************************************************************
 * @brief End of LineNumberPlainTextEdit.cpp
 *********************************************************************************************************************************/
