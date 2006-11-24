#include "astyle.h"
#include <Common/utils.h>
#include "astyle_interface.h"

using namespace std;
using namespace astyle;

namespace astyle {

ASBufferIterator::ASBufferIterator(const TCHAR * pBuffer)
{
	m_pBuffer = pBuffer;
}

ASBufferIterator::~ASBufferIterator()
{
}

bool ASBufferIterator::hasMoreLines() const
{
	return *m_pBuffer ? true : false;
}

string ASBufferIterator::nextLine()
{
	const TCHAR * p2 = m_pBuffer;
	const TCHAR * p1 = p2;

	while(*p2 && '\r' != *p2 && '\n' != *p2)
	{
		p2++;
	}
	m_pBuffer = p2;
	if(*m_pBuffer && ('\r' == *m_pBuffer || '\n' == *m_pBuffer)) m_pBuffer++;
	if(*m_pBuffer && ('\r' == *m_pBuffer || '\n' == *m_pBuffer) && (*(m_pBuffer-1) != *m_pBuffer)) m_pBuffer++;

	return string(p1, p2);
}

ASStreamIterator::ASStreamIterator(istream *in)
{
    inStream = in;
}

ASStreamIterator::~ASStreamIterator()
{
    delete inStream;
}


bool ASStreamIterator::hasMoreLines() const
{
    if (*inStream)
        return true;
    else
        return false;
}

/*
string ASStreamIterator::nextLine()
{
   char theInChar;
   char peekedChar;
   int  theBufferPosn = 0;
 
   //
   // treat '\n', '\r', '\n\r' and '\r\n' as an endline.
   //
   while (theBufferPosn < 2047 && inStream->get(theInChar))
   // while not eof
   {
      if (theInChar != '\n' && theInChar != '\r')
      {
	 buffer[theBufferPosn] = theInChar;
         theBufferPosn++;
      }
      else
      {
	peekedChar = inStream->peek();
	if (peekedChar != theInChar && (peekedChar == '\r' || peekedChar == '\n') )
         {
            inStream->get(theInChar);
         }
         break;
      }
   }
   buffer[theBufferPosn] = '\0';
 
   return string(buffer);
}
*/


string ASStreamIterator::nextLine()
{
    char *srcPtr;
    char *filterPtr;

    inStream->getline(buffer, 2047);
    srcPtr = filterPtr = buffer;

    while (*srcPtr != 0)
    {
        if (*srcPtr != '\r')
            *filterPtr++ = *srcPtr;
        srcPtr++;
    }
    *filterPtr = 0;

    return string(buffer);
}


} // end of namespace
