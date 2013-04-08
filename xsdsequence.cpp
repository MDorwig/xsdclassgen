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
	xsdType * base = NULL;
	if (m_parent->m_tag == type_complexExtension)
		base = ((xsdComplexExtension*)m_parent)->m_base;
	if (base != NULL)
		out.iprintln(indent,"struct %s : public %s",getCppName(),base->getCppName());
	else
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
	 * Parse Function
	 */
	out.iprintln(indent,"void Parse(xmlNodePtr node);");
	out.iprintln(indent,"void Write(xmlStream & out);");
	/*
	 * isset Function
	 */
	out.iprintln(indent,"bool isset() const ");
	out.iprintln(indent++,"{");
	if (base != NULL)
	{
		out.iprintln(indent,"if (%s::isset())",base->getCppName());
		out.iprintln(indent+1,"return true;") ;
	}
	for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		out.iprintln(indent,"if (%s.isset())",elem->getlvalue()) ;
		out.iprintln(indent+1,"return true;") ;
	}
	for (typeIterator ti = m_types.begin() ; ti != m_types.end() ; ti++)
	{
		xsdType * type = * ti ;
		if (type->m_tag == type_choice)
		{
			out.iprintln(indent,"if (m_choice.isset())");
			out.iprintln(indent+1,"return true;") ;
		}
	}
	out.iprintln(indent,"return false;");
	out.iprintln(--indent,"};\n");

	m_parent->GenAttrHeader(out,indent);
	m_elements.GenHeader(out,indent);
	m_types.GenHeader(out,indent,defaultstr,false);
	out.iprintln(--indent,"};\n");
}

void xsdSequence::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	xsdType * base = NULL;
	if (tascpp())
		return ;
	if (m_parent->m_tag == type_complexExtension)
		base = ((xsdComplexExtension*)m_parent)->m_base;
	if (base != NULL)
	{
		out.iprintln(1,"%s::Parse(node);",base->getCppName());
	}
	GenParserChildLoopStart(out,m_elements);
	GenElementCases(out,st,m_elements,false);
	m_types.GenImpl(out,st,defaultstr);
	GenParserChildLoopEnd(out);
}

void xsdSequence::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_elements.GenLocal(out,st);
}

void xsdSequence::GenWrite(CppFile & out,Symtab & st)
{
	GenWriteElements(out,m_elements);
	m_types.GenWrite(out,st);
}




