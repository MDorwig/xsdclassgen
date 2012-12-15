/*
 * xsdsequence.cpp
 *
 *  Created on: 15.12.2012
 *      Author: dorwig
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <libxml/parser.h>
#include "xsdclassgen.h"

void xsdSequence::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	std::list<xsdElement*>::iterator ei;
	out.iprintln(indent,"struct %s",getCppName());
	out.iprintln(indent++,"{");
	if (m_elements.hasPointer())
	{
		/*
		 * Constructor
		 */
		out.iprintln(indent,"%s()",getCppName());
		out.iprintln(indent++,"{");
		m_elements.GenInit(out,indent);
		out.iprintln(--indent,"}");
		/*
		 * Destructor
		 * delete any pointer members
		 */
		out.iprintln(indent,"~%s()",getCppName());
		out.iprintln(indent++,"{");
		m_elements.GenDelete(out,indent);
		out.iprintln(--indent,"}");
	}
	/*
	 *
	 */
	/*
	 * Parse Function
	 */
	out.iprintln(indent,"void Parse(xmlNodePtr node);");
	m_parent->GenAttrHeader(out,indent);
	m_elements.GenHeader(out,indent);
	m_types.GenHeader(out,indent,defaultstr,false);
	out.iprintln(--indent,"};\n");
}

void xsdSequence::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp())
		return ;
	GenParserChildLoopStart(out,st,m_elements,defaultstr,false);
	GenElementCases(out,st,m_elements,false);
	m_types.GenImpl(out,st,defaultstr);
	GenParserChildLoopEnd(out);
}

void xsdSequence::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_elements.GenLocal(out,st);
}





