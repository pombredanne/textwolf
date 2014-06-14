#include "textwolf/xmlscanner.hpp"
#include "textwolf/charset.hpp"
#include <iostream>
#include <string>

void output( const std::string& str)
{
	typedef textwolf::TextScanner<textwolf::CStringIterator,textwolf::charset::UTF8> Scanner;
	Scanner itr(src);
	
	for (; *itr; ++itr)
	{
		std::cout << std::hex << (unsigned int)*itr << std::dec << std::endl;
	}
}
