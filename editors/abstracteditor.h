#ifndef __ABSTRACTEDITOR_H_
#define __ABSTRACTEDITOR_H_

#include <QPlainTextEdit>
#include <QStringList>
#include <QColor>

class AbstractEditor : public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(bool bracketsMatchingEnabled READ isBracketsMatchingEnabled WRITE setBracketsMatchingEnabled)
    Q_PROPERTY(bool codeFoldingEnabled READ isCodeFoldingEnabled WRITE setCodeFoldingEnabled)
    Q_PROPERTY(bool lineNumbersVisible READ isLineNumbersVisible WRITE setLineNumbersVisible)
    Q_PROPERTY(bool textWrapEnabled READ isTextWrapEnabled WRITE setTextWrapEnabled)

public:
    AbstractEditor(QWidget* parent = NULL)
        : QPlainTextEdit(parent)
    {}

    typedef enum {
        Background,
        Normal,
        Comment,
        Number,
        String,
        Operator,
        Identifier,
        Keyword,
        BuiltIn,
        Sidebar,
        LineNumber,
        Cursor,
        Marker,
        BracketMatch,
        BracketError,
        FoldIndicator
    } ColorComponent;

    virtual void setColor(ColorComponent component, const QColor& color) = 0;

    virtual QStringList keywords(void) const = 0;
    virtual void setKeywords(const QStringList& keywords) = 0;

    virtual bool isBracketsMatchingEnabled(void) const = 0;
    virtual bool isCodeFoldingEnabled(void) const = 0;
    virtual bool isLineNumbersVisible(void) const = 0;
    virtual bool isTextWrapEnabled(void) const = 0;

    virtual bool isFoldable(int line) const = 0;
    virtual bool isFolded(int line) const = 0;

public slots:
    virtual void setBracketsMatchingEnabled(bool enable) = 0;
    virtual void setCodeFoldingEnabled(bool enable) = 0;
    virtual void setLineNumbersVisible(bool visible) = 0;
    virtual void setTextWrapEnabled(bool enable) = 0;

    virtual void fold(int line) = 0;
    virtual void unfold(int line) = 0;
    virtual void toggleFold(int line) = 0;

};

#endif // __ABSTRACTEDITOR_H_
