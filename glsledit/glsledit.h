/*
  This file is part of the Ofi Labs X2 project.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2010 Ariya Hidayat <ariya.hidayat@gmail.com>

  Modified March 2013 by ola@ct.de

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __GLSLEDIT_H_
#define __GLSLEDIT_H_

#include <QColor>
#include <QPlainTextEdit>
#include <QScopedPointer>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QTimer>

class GLSLEditPrivate;

class GLSLEdit: public QPlainTextEdit
{
    Q_OBJECT
    Q_PROPERTY(bool bracketsMatchingEnabled READ isBracketsMatchingEnabled WRITE setBracketsMatchingEnabled)
    Q_PROPERTY(bool codeFoldingEnabled READ isCodeFoldingEnabled WRITE setCodeFoldingEnabled)
    Q_PROPERTY(bool lineNumbersVisible READ isLineNumbersVisible WRITE setLineNumbersVisible)
    Q_PROPERTY(bool textWrapEnabled READ isTextWrapEnabled WRITE setTextWrapEnabled)

public:

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

    GLSLEdit(QWidget* parent = NULL);
    ~GLSLEdit();

    void setColor(ColorComponent component, const QColor& color);

    QStringList keywords(void) const;
    void setKeywords(const QStringList& keywords);

    bool isBracketsMatchingEnabled(void) const;
    bool isCodeFoldingEnabled(void) const;
    bool isLineNumbersVisible(void) const;
    bool isTextWrapEnabled(void) const;

    bool isFoldable(int line) const;
    bool isFolded(int line) const;

signals:
    void textChangedDelayed(void);

public slots:
    void updateSidebar(void);
    void mark(const QString &str, Qt::CaseSensitivity sens = Qt::CaseInsensitive);
    void setBracketsMatchingEnabled(bool enable);
    void setCodeFoldingEnabled(bool enable);
    void setLineNumbersVisible(bool visible);
    void setTextWrapEnabled(bool enable);
    void delayTextChange(void);

    void fold(int line);
    void unfold(int line);
    void toggleFold(int line);

protected:
    void resizeEvent(QResizeEvent*);
    void wheelEvent(QWheelEvent*);
    void keyPressEvent(QKeyEvent*);

private slots:
    void updateCursor(void);
    void updateSidebar(const QRect& rect, int d);

private:
    QTimer mTextChangedTimer;
    QScopedPointer<GLSLEditPrivate> d_ptr;
    Q_DECLARE_PRIVATE(GLSLEdit)
    Q_DISABLE_COPY(GLSLEdit)
};

#endif // __GLSLEDIT_H_
