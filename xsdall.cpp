/*
 * xsdall.cpp
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

void xsdAll::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
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
	if (isroot)
		out.iprintln(indent,"void Parse(xmlDocPtr node);");
	else
		out.iprintln(indent,"void Parse(xmlNodePtr node);");
	out.iprintln(indent,"void Write(xmlStream & out);");
	m_parent->GenAttrHeader(out,indent);
	m_elements.GenHeader(out,indent);
	out.iprintln(--indent,"};\n");
}

void xsdAll::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp())
		return ;
	GenParserChildLoopStart(out,m_elements);
	GenElementCases(out,st,m_elements,false);
	GenParserChildLoopEnd(out);
}

void xsdAll::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	m_elements.GenLocal(out,st);
}

void xsdAll::GenWrite(CppFile & out,Symtab & st)
{
	GenWriteElements(out,m_elements);
}
