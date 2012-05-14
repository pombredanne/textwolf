/*
---------------------------------------------------------------------
    The template library textwolf implements an input iterator on
    a set of XML path expressions without backward references on an
    STL conforming input iterator as source. It does no buffering
    or read ahead and is dedicated for stream processing of XML
    for a small set of XML queries.
    Stream processing in this Object refers to processing the
    document without buffering anything but the current result token
    processed with its tag hierarchy information.

    Copyright (C) 2010 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of textwolf can be found at 'http://github.com/patrickfrey/textwolf'
	For documentation see 'http://patrickfrey.github.com/textwolf'

--------------------------------------------------------------------
*/
///\file textwolf/xmlprinter.hpp
///\brief textwolf XML printer interface hiding character encoding properties

#ifndef __TEXTWOLF_XML_PRINTER_HPP__
#define __TEXTWOLF_XML_PRINTER_HPP__
#include "textwolf.hpp"
#include "textwolf/textscanner.hpp"
#include "textwolf/xmlparser.hpp"
#include "textwolf/xmltagstack.hpp"
#include "textwolf/xmlattributes.hpp"
#include "textwolf/cstringiterator.hpp"
#include <cstring>
#include <cstdlib>

///\namespace textwolf
///\brief Toplevel namespace of the library
namespace textwolf {

template <class BufferType>
struct XMLPrinterBase
{
	typedef bool (*PrintProc)( void* obj, const char* elemptr, std::size_t elemsize, BufferType& buf);
	typedef bool (*PrintProcEmpty)( void* obj, BufferType& buf);
	typedef void* (*CopyObj)( void* obj);
	typedef void (*DeleteObj)( void* obj);

	struct MethodTable
	{
		PrintProc m_printOpenTag;
		PrintProcEmpty m_printCloseTag;
		PrintProc m_printAttribute;
		PrintProc m_printValue;
		CopyObj m_copy;
		DeleteObj m_del;

		MethodTable() :m_printOpenTag(0),m_printCloseTag(0),m_printAttribute(0),m_printValue(0),m_copy(0),m_del(0){}
	};

	static void parseEncoding( std::string& dest, const std::string& src)
	{
		dest.clear();
		std::string::const_iterator cc=src.begin();
		for (; cc != src.end(); ++cc)
		{
			if (*cc <= ' ') continue;
			if (*cc == '-') continue;
			if (*cc == ' ') continue;
			dest.push_back( ::tolower( *cc));
		}
	}
};

///\class XMLPrinterObject
///\brief Character encoding dependent printing object
///\tparam IOCharset Character set encoding of input and output
///\tparam AppCharset Character set encoding of the application processor
///\tparam BufferType STL back insertion sequence to use for printing output
template <class BufferType, class IOCharset, class AppCharset, class XMLAttributes>
class XMLPrinterObject
{
private:
	typedef typename XMLPrinterBase<BufferType>::MethodTable MethodTable;

	///\class This
	struct This
	{
		This( const XMLAttributes& a)
			:m_state(Init),m_attributes(a){}

		This( const This& o)
			:m_state(o.m_state),m_buf(o.m_buf),m_tagstack(o.m_tagstack),m_attributes(o.m_attributes){}

		///\brief Prints a character string to an STL back insertion sequence buffer in the IO character set encoding
		///\param [in] src pointer to string to print
		///\param [in] srcsize size of src in bytes
		static void printToBuffer( const char* src, std::size_t srcsize, BufferType& buf)
		{
			CStringIterator itr( src, srcsize);
			TextScanner<CStringIterator,AppCharset> ts( itr);

			UChar ch;
			while ((ch = ts.chr()) != 0)
			{
				IOCharset::print( ch, buf);
				++ts;
			}
		}

		///\brief print a character substitute or the character itself
		///\param [in,out] buf buffer to print to
		///\param [in] nof_echr number of elements in echr and estr
		///\param [in] echr ASCII characters to substitute
		///\param [in] estr ASCII strings to substitute with (array parallel to echr)
		static void printEsc( char ch, BufferType& buf, unsigned int nof_echr, const char* echr, const char** estr)
		{
			const char* cc = (const char*)memchr( echr, ch, nof_echr);
			if (cc)
			{
				unsigned int ii = 0;
				const char* tt = estr[ cc-echr];
				while (tt[ii]) IOCharset::print( tt[ii++], buf);
			}
			else
			{
				IOCharset::print( ch, buf);
			}
		}

		///\brief print a value with some characters replaced by a string
		///\param [in] src pointer to attribute value string to print
		///\param [in] srcsize size of src in bytes
		///\param [in,out] buf buffer to print to
		///\param [in] nof_echr number of elements in echr and estr
		///\param [in] echr ASCII characters to substitute
		///\param [in] estr ASCII strings to substitute with (array parallel to echr)
		static void printToBufferSubstChr( const char* src, std::size_t srcsize, BufferType& buf, unsigned int nof_echr, const char* echr, const char** estr)
		{
			CStringIterator itr( src, srcsize);
			textwolf::TextScanner<CStringIterator,AppCharset> ts( itr);

			textwolf::UChar ch;
			while ((ch = ts.chr()) != 0)
			{
				if (ch < 128)
				{
					printEsc( (char)ch, buf, nof_echr, echr, estr);
				}
				else
				{
					IOCharset::print( ch, buf);
				}
				++ts;
			}
		}

		///\brief print attribute value string
		///\param [in] src pointer to attribute value string to print
		///\param [in] srcsize size of src in bytes
		///\param [in,out] buf buffer to print to
		static void printToBufferAttributeValue( const char* src, std::size_t srcsize, BufferType& buf)
		{
			enum {nof_echr = 12};
			static const char* estr[nof_echr] = {"&lt;", "&gt;", "&apos;", "&quot;", "&amp;", "&#0;", "&#8;", "&#9;", "&#10;", "&#13;"};
			static const char echr[nof_echr+1] = "<>'\"&\0\b\t\n\r";
			IOCharset::print( '"', buf);
			printToBufferSubstChr( src, srcsize, buf, nof_echr, echr, estr);
			IOCharset::print( '"', buf);
		}

		///\brief print content value string
		///\param [in] src pointer to content string to print
		///\param [in] srcsize size of src in bytes
		///\param [in,out] buf buffer to print to
		static void printToBufferContent( const char* src, std::size_t srcsize, BufferType& buf)
		{
			enum {nof_echr = 6};
			static const char* estr[nof_echr] = {"&lt;", "&gt;", "&amp;", "&#0;", "&#8;"};
			static const char echr[nof_echr+1] = "<>&\0\b";
			printToBufferSubstChr( src, srcsize, buf, nof_echr, echr, estr);
		}

		///\brief Prints a character to an STL back insertion sequence buffer in the IO character set encoding
		///\param [in] ch character to print
		///\param [in,out] buf buffer to print to
		static void printToBuffer( char ch, BufferType& buf)
		{
			IOCharset::print( (textwolf::UChar)(unsigned char)ch, buf);
		}

		void printHeader( BufferType& buf)
		{
			std::string enc = m_attributes.getEncoding();
			printToBuffer( "<?xml version=\"1.0\" encoding=\"", 30, buf);
			printToBuffer( enc.c_str(), enc.size(), buf);
			printToBuffer( "\" standalone=\"yes\"?>\n", 21, buf);
			m_state = Content;
		}

		void exitTagContext( BufferType& buf)
		{
			if (m_state != Content)
			{
				if (m_state == Init)
				{
					printHeader( buf);
				}
				printToBuffer( '>', buf);
				m_state = Content;
			}
		}

		bool printOpenTag( const char* src, std::size_t srcsize, BufferType& buf)
		{
			exitTagContext( buf);
			printToBuffer( '<', buf);
			printToBuffer( (const char*)src, srcsize, buf);

			m_tagstack.push( src, srcsize);
			m_state = TagElement;
			return true;
		}

		bool printAttribute( const char* src, std::size_t srcsize, BufferType& buf)
		{
			if (m_state == TagElement)
			{
				printToBuffer( ' ', buf);
				printToBuffer( (const char*)src, srcsize, buf);
				printToBuffer( '=', buf);
				m_state = TagAttribute;
				return true;
			}
			return false;
		}

		bool printValue( const char* src, std::size_t srcsize, BufferType& buf)
		{
			if (m_state == TagAttribute)
			{
				printToBuffer( ' ', buf);
				printToBufferAttributeValue( (const char*)src, srcsize, buf);
				m_state = TagElement;
				return true;
			}
			else
			{
				exitTagContext( buf);
				printToBufferContent( (const char*)src, srcsize, buf);
			}
			return false;
		}

		bool printCloseTag( BufferType& buf)
		{
			const void* cltag;
			std::size_t cltagsize;

			if (m_tagstack.top( cltag, cltagsize) || !cltagsize)
			{
				return false;
			}
			if (m_state == TagElement)
			{
				printToBuffer( '/', buf);
				printToBuffer( '>', buf);
				m_state = Content;
			}
			else if (m_state != Content)
			{
				return false;
			}
			else
			{
				printToBuffer( '<', buf);
				printToBuffer( '/', buf);
				printToBuffer( (const char*)cltag, cltagsize, buf);
				printToBuffer( '>', buf);
			}
			m_tagstack.pop();
			return true;
		}

	private:
		enum State
		{
			Init,
			Content,
			TagAttribute,
			TagElement
		};
		State m_state;					//< output state
		BufferType m_buf;				//< element output  buffer
		TagStack m_tagstack;				//< tag name stack of open tags
		XMLAttributes m_attributes;			//< xml encoding
	};

public:
	static void* createObj( const XMLAttributes& a)
	{
		return new This( a);
	}

	static void* copyObj( void* obj_)
	{
		This* obj = (This*)obj_;
		This* rt = new This( *obj);
		return rt;
	}

	static bool printOpenTag( void* obj_, const char* src, std::size_t srcsize, BufferType& buf)
	{
		This* obj = (This*)obj_;
		return obj->printOpenTag( src, srcsize, buf);
	}

	static bool printCloseTag( void* obj_, BufferType& buf)
	{
		This* obj = (This*)obj_;
		return obj->printCloseTag( buf);
	}

	static bool printAttribute( void* obj_, const char* src, std::size_t srcsize, BufferType& buf)
	{
		This* obj = (This*)obj_;
		return obj->printAttribute( src, srcsize, buf);
	}

	static bool printValue( void* obj_, const char* src, std::size_t srcsize, BufferType& buf)
	{
		This* obj = (This*)obj_;
		return obj->printValue( src, srcsize, buf);
	}

	static void deleteObj( void* obj)
	{
		delete (This*)obj;
	}

	static void* create( MethodTable& mt, const XMLAttributes& a)
	{
		mt.m_printOpenTag = printOpenTag;
		mt.m_printCloseTag = printCloseTag;
		mt.m_printAttribute = printAttribute;
		mt.m_printValue = printValue;
		mt.m_copy = copyObj;
		mt.m_del = deleteObj;
		return createObj( a);
	}

	static void* copy( MethodTable& mt, void* obj)
	{
		mt.m_printOpenTag = printOpenTag;
		mt.m_printCloseTag = printCloseTag;
		mt.m_printAttribute = printAttribute;
		mt.m_printValue = printValue;
		mt.m_del = deleteObj;
		return copyObj( obj);
	}
};


///\brief Class for XML printing to a buffer
///\tparam BufferType type to use as buffer (STL back insertion interface)
template <class BufferType, class XMLAttributes=DefaultXMLAttributes>
class XMLPrinter :public XMLPrinterBase<BufferType>
{
public:
	XMLPrinter( const XMLAttributes& a)
		:m_obj(0),m_attributes(a){}

	XMLPrinter( const XMLPrinter& o)
		:m_mt(o.m_mt),m_obj(0),m_attributes(o.m_attributes)
	{
		m_obj = m_mt.m_copy( m_obj);
	}

	~XMLPrinter()
	{
		m_mt.m_del( m_obj);
	}

	bool printOpenTag( const char* src, std::size_t srcsize, BufferType& buf)
	{
		if (!m_obj && !createPrinter()) return false;
		return m_mt.m_printOpenTag( m_obj, src, srcsize, buf);
	}

	bool printCloseTag( const char* src, std::size_t srcsize, BufferType& buf)
	{
		if (!m_obj && !createPrinter()) return false;
		return m_mt.m_printCloseTag( m_obj, buf);
	}

	bool printAttribute( const char* src, std::size_t srcsize, BufferType& buf)
	{
		if (!m_obj && !createPrinter()) return false;
		return m_mt.m_printAttribute( m_obj, src, srcsize, buf);
	}

	bool printValue( const char* src, std::size_t srcsize, BufferType& buf)
	{
		if (!m_obj && !createPrinter()) return false;
		return m_mt.m_printValue( m_obj, src, srcsize, buf);
	}

	bool createPrinter()
	{
		std::string enc;
		parseEncoding( enc, m_attributes.getEncoding());

		if (m_obj)
		{
			m_mt.m_del( m_obj);
			m_obj = 0;
		}

		if ((enc.size() >= 8 && std::memcmp( enc.c_str(), "isolatin", enc.size())== 0)
		||  (enc.size() >= 7 && std::memcmp( enc.c_str(), "iso8859", enc.size()) == 0))
		{
			m_obj = XMLPrinterObject<BufferType, charset::IsoLatin1, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc.size() == 0 || enc == "utf8")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UTF8, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc == "utf16" || enc == "utf16be")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UTF16BE, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc == "utf16le")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UTF16LE, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc == "ucs2" || enc == "ucs2be")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UCS2BE, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc == "ucs2le")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UCS2LE, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc == "ucs4" || enc == "ucs4be")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UCS4BE, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else if (enc == "ucs4le")
		{
			m_obj = XMLPrinterObject<BufferType, charset::UCS4LE, charset::UTF8, XMLAttributes>::create( m_mt, m_attributes);
		}
		else
		{
			return false;
		}
	}

private:
	typedef typename XMLPrinterBase<BufferType>::MethodTable MethodTable;

	MethodTable m_mt;				//< method table of m_obj
	void* m_obj;					//< pointer to parser objecct
	XMLAttributes m_attributes;			//< xml encoding
};

} //namespace
#endif
