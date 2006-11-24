#ifndef ASTYLE_INTERFACE_H__
#define ASTYLE_INTERFACE_H__

#include <iostream>
#include <fstream>
#include "astyle.h"

namespace astyle {

class ASBufferIterator : public ASSourceIterator
{
    public:
        ASBufferIterator(const char * pBuffer);
        virtual ~ASBufferIterator();
        bool hasMoreLines() const;
        string nextLine();

    private:
        const char * m_pBuffer;
};

class ASStreamIterator : public ASSourceIterator
{
    public:
        ASStreamIterator(istream *in);
        virtual ~ASStreamIterator();
        bool hasMoreLines() const;
        string nextLine();

    private:
        istream * inStream;
        char buffer[2048];
};


} // end of namespace





#endif

