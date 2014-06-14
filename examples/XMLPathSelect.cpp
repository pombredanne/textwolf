#include "textwolf/xmlscanner.hpp"
#include "textwolf/xmlpathselect.hpp"
#include "textwolf/charset.hpp"
#include <iostream>
#include <string>

void output( const std::string& str)
{
	typedef textwolf::charset::UTF8 Encoding;
	typedef textwolf::CStringIterator Iterator;
	typedef textwolf::XMLScanner<Iterator,Encoding,Encoding,std::string> Scanner;
	typedef textwolf::XMLPathSelect<Encoding> Selector;

	textwolf::XMLPathSelectAutomaton<Encoding> atm;
	(*atm)["address"]("name") = 1;	 //... assign 1 to matches of /address/@name
	(*atm)["address"]("street") = 2; //... assign 2 to matches of /address/@street

	Scanner scanner( str);
	Selector selector( &atm);

	// Fetch the input elements, feed them to the selector and iterate on the result dropping out:
	Scanner::iterator ci,ce;
	for (ci=scanner.begin(),ce=scanner.end(); ci!=ce; ci++)
	{
		std::string elem = std::string( itr->content(), itr->size());
		Selector::iterator itr = selector.push( ci->type(), elem), end = selector.end();

		for (; itr!=end; itr++)
		{
			std::cout << *itr << ": " << itr->name() << elem << std::endl;
		}
	}
}
