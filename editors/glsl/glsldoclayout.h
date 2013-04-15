#ifndef __GLSLDOCLAYOUT_H_
#define __GLSLDOCLAYOUT_H_

#include <QTextDocument>
#include <QPlainTextDocumentLayout>

class GLSLDocLayout: public QPlainTextDocumentLayout
{
public:
    GLSLDocLayout(QTextDocument* doc);
    void forceUpdate(void);
};


#endif // __GLSLDOCLAYOUT_H_
