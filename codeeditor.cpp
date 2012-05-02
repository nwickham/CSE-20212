#include <QPlainTextEdit>
#include <QPainter>
#include <QTextBlock>
#include <QFont>
#include <QMenu>
#include <QContextMenuEvent>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QCoreApplication>
#include <QMessageBox>

#include <QtGui>

#include "highlighter.h"
#include "codeeditor.h"
#include "linenumberarea.h"
#include "spellchecker.h"

SpellChecker *spellChecker;
QList<QAction *> suggestionWordsList;
QAction *suggestedWord,
        *addToDict;
QString selectedWord;

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{

    lineNumberArea = new LineNumberArea(this);
dolinenums = 0;
dolinelight = 0;
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    QString dictPath = "C:/Users/Nate/Desktop/hunspell/en_US";
    QString userDict= "C:/Users/Nate/Desktop/hunspell/userDict.txt";
    spellChecker = new SpellChecker(dictPath, userDict);

    addToDict = new QAction(tr("Add to dictionary"),this);
    connect(addToDict,SIGNAL(triggered()),this,SLOT(addToDictionary()));

    for (int i = 0; i < suggestionWordsList.count(); ++i)
    {
        suggestionWordsList[i] = new QAction(this);
        suggestionWordsList[i]->setVisible(false);
        connect(suggestionWordsList[i], SIGNAL(triggered()),this, SLOT(replaceWord()));
    }

  //  MyTextHighlighter= new CTextSyntaxHighlighter(this->document());
}

CodeEditor::CodeEditor(const QString & text, QWidget *parent) : QPlainTextEdit(text, parent)
{
    lineNumberArea = new LineNumberArea(this);
dolinenums = 0;
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::uninit()
{
updateLineNumberAreaWidth(-1);
highlightCurrentLine();
}

void CodeEditor::init()
{
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::setdolines(int x)
{
  dolinenums = x;
}

void CodeEditor::setdolinelight(int x)
{
    dolinelight = x;
}

int CodeEditor::lineNumberAreaWidth()
{
    if(dolinenums)
    {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
    }
    return 0;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::replaceWord()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
        insertPlainText(action->text());
}

void CodeEditor::addToDictionary()
{
    spellChecker->addToUserWordlist(selectedWord);
    insertPlainText(selectedWord);
}

void CodeEditor::updateTextSpeller()
{
    QTextCharFormat highlightFormat;
    highlightFormat.setUnderlineColor(QColor("red"));
    highlightFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

    // save the position of the current cursor
    QTextCursor oldCursor = textCursor();

    // create a new cursor to walk through the text
    QTextCursor cursor(document());

    // Don't call cursor.beginEditBlock(), as this prevents the rewdraw after changes to the content
    // cursor.beginEditBlock();
    while(!cursor.atEnd()) {
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor, 1);
        QString word = cursor.selectedText();

        // Workaround for better recognition of words
        // punctuation etc. does not belong to words
        while(!word.isEmpty() && !word.at(0).isLetter() && cursor.anchor() < cursor.position())
        {
            int cursorPos = cursor.position();
            cursor.setPosition(cursor.anchor() + 1, QTextCursor::MoveAnchor);
            cursor.setPosition(cursorPos, QTextCursor::KeepAnchor);
            word = cursor.selectedText();
        }

        if(!word.isEmpty() && !spellChecker->spell(word))
        {
            QTextCursor tmpCursor(cursor);
            tmpCursor.setPosition(cursor.anchor());
            setTextCursor(tmpCursor);
            ensureCursorVisible();

            // highlight the unknown word
            QTextEdit::ExtraSelection es;
            es.cursor = cursor;
            es.format = highlightFormat;

            QList<QTextEdit::ExtraSelection> esList;
            esList << es;
            setExtraSelections(esList);

            // reset the word highlight
//            esList.clear();
            setExtraSelections(esList);

                    }
        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor, 1);
    }
    //cursor.endEditBlock();
    setTextCursor(oldCursor);
//    QMessageBox::warning(this,"wait","press OK!");
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
    selectedWord.clear();

    QTextCursor cursor= textCursor();
    cursor.setPosition(cursorForPosition(event->pos()).position());
    cursor.select(QTextCursor::WordUnderCursor);
    setTextCursor(cursor);

    selectedWord=cursor.selection().toPlainText();


    QMenu *editContextMenu = QPlainTextEdit::createStandardContextMenu();

    editContextMenu->addSeparator();
    QStringList suggestions = spellChecker->suggest(selectedWord);

    suggestionWordsList.clear();

    for(int i=0;i<suggestions.count();i++)
    {
        suggestedWord = new QAction(this);
        suggestedWord->setText(suggestions.at(i));
        connect(suggestedWord, SIGNAL(triggered()),this, SLOT(replaceWord()));
        suggestionWordsList.append(suggestedWord);
    }
    editContextMenu->addActions(suggestionWordsList);

    if(!cursor.selection().isEmpty())
    {
        editContextMenu->addSeparator();
        editContextMenu->addAction(addToDict);
    }

    editContextMenu->exec(event->globalPos());
}

void CodeEditor::highlightCurrentLine()
{

    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(250);
        if(dolinelight)
        lineColor.setAlpha(50);
        else
            lineColor.setAlpha(0);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);

}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
 {
    if(dolinenums)
    {
     QPainter painter(lineNumberArea);
     painter.fillRect(event->rect(), BackgroundColor);
     QTextBlock block = firstVisibleBlock();
          int blockNumber = block.blockNumber();
          int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
          int bottom = top + (int) blockBoundingRect(block).height();
          while (block.isValid() && top <= event->rect().bottom()) {
              if (block.isVisible() && bottom >= event->rect().top()) {
                  QString number = QString::number(blockNumber + 1);
                  painter.setPen(NumbersColor);
                  painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                                   Qt::AlignRight, number);
              }

              block = block.next();
              top = bottom;
              bottom = top + (int) blockBoundingRect(block).height();
              ++blockNumber;
          }
    }
      }
void CodeEditor::setColors(int x)
{
    if(x ==1)
    {
BackgroundColor.setRgb(214, 214, 214);
NumbersColor.setRgb(255, 0, 255);
    }
    else if(x == 2)
    {
        BackgroundColor.setRgb(39,64,139);
        NumbersColor.setRgb(176, 196, 222);
    }
    else if(x == 3)
    {
        BackgroundColor.setRgb(218, 165, 32);
        NumbersColor.setRgb(139, 137, 137);
    }
}


void CodeEditor::matchParentheses()
{
    QList<QTextEdit::ExtraSelection> selections;
    setExtraSelections(selections);

    TextBlockData *data = static_cast<TextBlockData *>(textCursor().block().userData());

    if (data) {
        QVector<ParenthesisInfo *> infos = data->parentheses();

        int pos = textCursor().block().position();
        for (int i = 0; i < infos.size(); ++i) {
            ParenthesisInfo *info = infos.at(i);

            int curPos = textCursor().position() - textCursor().block().position();
            if (info->position == curPos - 1 && info->character == '{') {
                if (matchLeftParenthesis(textCursor().block(), i + 1, 0))
                    createParenthesisSelection(pos + info->position);
            } else if (info->position == curPos - 1 && info->character == '}') {
                if (matchRightParenthesis(textCursor().block(), i - 1, 0))
                    createParenthesisSelection(pos + info->position);
            }
        }
    }
}

bool CodeEditor::matchLeftParenthesis(QTextBlock currentBlock, int i, int numLeftParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<ParenthesisInfo *> infos = data->parentheses();

    int docPos = currentBlock.position();
    for (; i < infos.size(); ++i) {
        ParenthesisInfo *info = infos.at(i);

        if (info->character == '{') {
            ++numLeftParentheses;
            continue;
        }

        if (info->character == '}' && numLeftParentheses == 0) {
            createParenthesisSelection(docPos + info->position);
            return true;
        } else
            --numLeftParentheses;
    }

    currentBlock = currentBlock.next();
    if (currentBlock.isValid())
        return matchLeftParenthesis(currentBlock, 0, numLeftParentheses);

    return false;
}

bool CodeEditor::matchRightParenthesis(QTextBlock currentBlock, int i, int numRightParentheses)
{
    TextBlockData *data = static_cast<TextBlockData *>(currentBlock.userData());
    QVector<ParenthesisInfo *> parentheses = data->parentheses();

    int docPos = currentBlock.position();
    for (; i > -1 && parentheses.size() > 0; --i) {
        ParenthesisInfo *info = parentheses.at(i);
        if (info->character == '}') {
            ++numRightParentheses;
            continue;
        }
        if (info->character == '{' && numRightParentheses == 0) {
            createParenthesisSelection(docPos + info->position);
            return true;
        } else
            --numRightParentheses;
    }

    currentBlock = currentBlock.previous();
    if (currentBlock.isValid())
        return matchRightParenthesis(currentBlock, 0, numRightParentheses);

    return false;
}

void CodeEditor::createParenthesisSelection(int pos)
{
    QList<QTextEdit::ExtraSelection> selections = extraSelections();

    QTextEdit::ExtraSelection selection;
    QTextCharFormat format = selection.format;
    format.setForeground(Qt::red);
    format.setFontWeight(QFont::Bold);
    selection.format = format;

    QTextCursor cursor = textCursor();
    cursor.setPosition(pos);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    selection.cursor = cursor;

    selections.append(selection);

    setExtraSelections(selections);
}

void CodeEditor::deleteHighlighter(bool x)
{
    int y;
    if(x == false)// && MyTextHighlighter)
    {
        delete MyTextHighlighter;
        y = 0;
    }
    else if(x == true)// && !MyTextHighlighter)
    {
        MyTextHighlighter= new CTextSyntaxHighlighter(this->document());
        y = 1;
    }
    qDebug() << y << endl;
}
