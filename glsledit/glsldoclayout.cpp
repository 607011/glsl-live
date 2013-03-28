#include "glsldoclayout.h"

GLSLDocLayout::GLSLDocLayout(QTextDocument* doc)
    : QPlainTextDocumentLayout(doc)
{
    /* ... */
}

void GLSLDocLayout::forceUpdate(void)
{
    emit documentSizeChanged(documentSize());
}


