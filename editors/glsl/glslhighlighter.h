#ifndef __GLSLHIGHLIGHTER_H_
#define __GLSLHIGHLIGHTER_H_

#include <QTextDocument>
#include <QString>
#include <QStringList>
#include <QList>
#include <QSet>
#include <QHash>
#include <QColor>
#include <QSyntaxHighlighter>
#include <QTextBlockUserData>

#include "glsledit.h"

class GLSLHighlighter : public QSyntaxHighlighter
{
public:
    GLSLHighlighter(QTextDocument* parent = NULL);
    void setColor(GLSLEdit::ColorComponent component, const QColor& color);
    void mark(const QString& str, Qt::CaseSensitivity caseSensitivity);

    QStringList keywords(void) const;
    void setKeywords(const QStringList& keywords);

protected:
    void highlightBlock(const QString& text);

private:
    QSet<QString> m_keywords;
    QSet<QString> m_knownIds;
    QHash<GLSLEdit::ColorComponent, QColor> m_colors;
    QString m_markString;
    Qt::CaseSensitivity m_markCaseSensitivity;
};


class GLSLBlockData: public QTextBlockUserData
{
public:
    QList<int> bracketPositions;
};




#endif // __GLSLHIGHLIGHTER_H_
