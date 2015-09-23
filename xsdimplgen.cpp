/*
 * xsdparsergen.cpp
 *
 *  Created on: 03.12.2012
 *      Author: dorwig
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <libxml/parser.h>
#include "xsdclassgen.h"

void MoveAttributes(xsdAttrList & srclist,xsdAttrList & destlist)
{
	attrIterator ai ;
	ai = srclist.begin();
	while(ai != srclist.end())
	{
		xsdAttribute * attr = *ai ;
		printf("MoveAttributes %s\n",attr->getName());
		srclist.remove(attr);
		destlist.push_back(attr);
		ai = srclist.begin();
	}
}

void GenElementCases(CppFile & out,Symtab & st,xsdElementList & elements,bool ischoice)
{
	for (elementIterator ei = elements.begin() ; ei != elements.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		Symbol * s = st.find(elem->m_name.c_str());
		if (s != NULL)
		{
			int id = 4 ;
			out.iprintln(id,"case sy_%s:",s->m_cname.c_str());
			if (ischoice)
			{
				out.iprintln(id++,"{");
				out.iprintln(id,"%s.m_selected = %s::%s;",elem->getChoiceVarname(),elem->getChoiceTypename(),elem->m_choice_selector.c_str());;
				if (elem->isArray())
				{
					out.iprintln(id,"%s tmp;",elem->m_type->getCppName());
					elem->m_type->GenAssignment(out,id,"tmp","getContent(child)");
					if (elem->isPtr())
					{
						out.iprintln(id,"if (%s.%s == NULL)",elem->getChoiceVarname(),elem->getName());
						out.iprintln(id+1,"%s.%s = new xs_array<%s>;",elem->getChoiceVarname(),elem->getCppName(),elem->m_type->getCppName());
						out.iprintln(id,"%s.%s->add(tmp);",elem->getChoiceVarname(),elem->getCppName());
					}
					else
						out.iprintln(id,"%s.%s.add(tmp);",elem->getChoiceVarname(),elem->getCppName());
				}
				else
				{
					elem->m_type->GenAssignment(out,id,*elem,"getContent(child)");
				}
				out.iprintln(--id,"}");
			}
			else
			{
				if (elem->m_type != NULL)
				{
					int indent = 5 ;
					const char * typname = elem->m_type->getCppName();
					if (elem->isArray())
					{
						out.iprintln(indent++,"{");
						out.iprintln(indent,  "%s tmp ;",typname);
						elem->m_type->GenAssignment(out,indent,"tmp","getContent(child)");
						if (elem->isPtr())
						{
							out.iprintln(indent,"if (%s == NULL)",elem->getCppName());
							out.iprintln(indent+1,"%s = new xs_array<%s>();",elem->getCppName(),typname);
						}
						out.iprintln(indent,"%s.add(tmp);",elem->getlvalue());
						out.iprintln(--indent,"}");
					}
					else
					{
						elem->m_type->GenAssignment(out,indent,*elem,"getContent(child)");
					}
				}
			}
			out.iprintln(id,"break;");
		}
	}
}

void GenWriteElementCases(CppFile & out,Symtab & st,xsdElementList & elements)
{
	for (elementIterator ei = elements.begin() ; ei != elements.end() ; ei++)
	{
		int indent = 2 ;
		xsdElement * elem = *ei ;
		Symbol * s = st.find(elem->getName());
		if (s != NULL)
		{
			const char * name = elem->getName();
			out.iprintln(indent,  "case %s::%s:",elem->getChoiceTypename(),elem->m_choice_selector.c_str());
			out.iprintln(indent++,"{");
			out.iprintln(indent  ,  "%s * sel = %s.%s;",elem->m_typename->getCppName(),elem->getChoiceVarname(),elem->getCppName());
			out.iprintln(indent,    "if (sel->isset())");
			out.iprintln(indent++,  "{");
			if (elem->hasAttributes())
			{
				out.iprintln(indent,    "out.putrawstring(\"<%s\");",name);
				elem->GenWriteAttr(out,indent);
				out.iprintln(indent,    "out.putrawstring(\">\");");
			}
			else
				out.iprintln(indent,    "out.putrawstring(\"<%s>\");",name);
			out.iprintln(indent,      "sel->Write(out);");
			out.iprintln(indent,      "out.putrawstring(\"</%s>\");",name);
			out.iprintln(--indent,   "}");
			out.iprintln(--indent,"}");
			out.iprintln(indent,"break;");
		}
	}
	out.iprintln(2,"default:");
	out.iprintln(2,"break;");
}

void xsdElement::GenWriteAttr(CppFile & out,int indent)
{
	if (m_type != NULL)
		m_type->GenWriteAttr(out,indent,this);
}

void GenWriteElem(CppFile & out,int indent,xsdElement * elem)
{
	xsdType * type = elem->m_type;
	const char * name = elem->getName();
	std::string var = elem->getlvalue();
	std::string cvar = var ;
	if (elem->m_type == NULL)
	{
		return ;
	}
	if (elem->isArray())
	{
		var += "[i]";
		cvar = var ;
	}
	else
	{
		if (type->m_tag == type_complex)
		{
			xsdComplexType * ctype = (xsdComplexType*)type ;
			if (ctype->m_type != NULL && ctype->m_type->m_tag == type_choice)
			{
				xsdChoice * choice = (xsdChoice*)ctype->m_type;
				cvar += "." ;
				cvar += choice->getVarname();
			}
		}
	}
	if (elem->isPtr())
		out.iprintln(indent,   "if (%s != NULL && %s.isset())",elem->getCppName(),cvar.c_str());
	else
		out.iprintln(indent,   "if (%s.isset())",cvar.c_str());
	out.iprintln(indent++,"{");
	if (elem->m_type->isString())
	{
		out.iprintln(indent,    "if (%s.empty())",var.c_str());
		out.iprintln(indent+1,    "out.putrawstring(\"<%s/>\");",name);
		out.iprintln(indent,    "else");
		out.iprintln(indent++,  "{");
		out.iprintln(indent,      "out.putrawstring(\"<%s>\");",name);
	  out.iprintln(indent,      "%s.Write(out);",var.c_str());
	  out.iprintln(indent,      "out.putrawstring(\"</%s>\");",name);
		out.iprintln(--indent,  "}");
	}
	else
	{
		if (elem->hasAttributes())
		{
			out.iprintln(indent,"out.putrawstring(\"<%s\");",name);
			elem->GenWriteAttr(out,indent);
			out.iprintln(indent,"out.putrawstring(\">\");");
		}
		else
			out.iprintln(indent,"out.putrawstring(\"<%s>\");",name);
		out.iprintln(indent,"%s.Write(out);",var.c_str());
		out.iprintln(indent,"out.putrawstring(\"</%s>\");",elem->getName());
	}
	out.iprintln(--indent,"}");
}

void xsdElement::GenWrite(CppFile & out)
{
	if (isArray())
	{
		int lvl = 1 ;
		if (isPtr())
		{
			out.iprintln(lvl,"if (%s != NULL)",getCppName());
			out.iprintln(lvl,"{");
			lvl++;
		}
		out.iprintln(lvl,"for(size_t i = 0 ; i < %s.count() ; i++)",getlvalue());
		out.iprintln(lvl,"{");
		GenWriteElem(out,lvl+1,this);
		out.iprintln(lvl,"}");
		if (isPtr())
			out.iprintln(lvl-1,"}");
	}
	else
		GenWriteElem(out,1,this);
}

void GenWriteElements(CppFile & out,xsdElementList & elements)
{
	for (elementIterator ei = elements.begin() ; ei != elements.end() ; ei++)
	{
		(*ei)->GenWrite(out);
	}
}

void GenParserChildLoopStart(CppFile & out,xsdElementList & elements)
{
	out.iprintln(1,"for_each_child(child,node)");
	out.iprintln(1,"{");
	out.iprintln(2,"if (child->type == XML_ELEMENT_NODE)");
	out.iprintln(2,"{");
	out.iprintln(3,"symbols kw = Lookup(child->name);");
	out.iprintln(3,"switch(kw)");
	out.iprintln(3,"{");
}

void GenParserChildLoopEnd(CppFile & out)
{
	out.iprintln(4,"default:");
	out.iprintln(4,"break;");
	out.iprintln(3,"}");
	out.iprintln(2,"}");
	out.iprintln(1,"}");
}

void xsdSimpleExtension::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	if (!tashdr())
	{
		if (m_exttype != NULL)
			m_exttype->GenHeader(out,indent,defaultstr);
		else
		{
			out.iprintln(indent,"struct %s : public %s",getCppName(),m_base->getCppName());
			out.iprintln(indent++,"{");
			if (HasFixedAttributes())
			{
				out.iprintln(indent,"%s()",getCppName());
				out.iprintln(indent,"{");
				GenInitFixedAttr(out,indent+1);
				out.iprintln(indent,"}");
			}

			out.iprintln(indent,"void Parse(xmlNodePtr node);");
			out.iprintln(indent,"void Write(xmlStream & out);");
			if (!m_attributes.empty())
			{
				out.iprintln(indent,"bool isset() const");
				out.iprintln(indent++,"{");
				out.iprintf(indent,"return %s::isset()",m_base->getCppName());
				for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
				{
						out.printf(" || %s.isset()",(*ai)->getCppName());
				}
				out.println(";");
				out.iprintln(--indent,"}");
			}
			GenAttrHeader(out,indent);
			out.iprintln(--indent,"};");
		}
	}
}

void xsdSimpleExtension::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp())
		return ;
	if (m_base->isStruct())
		out.iprintln(1,"%s::Parse(node);",m_base->getCppName());
	if (m_exttype != NULL)
		m_exttype->GenImpl(out,st,defaultstr);
	if (m_base != NULL && (m_base->isScalar() || m_base->isString() || m_base->isEnum()))
	{
		out.iprintln(1,"sets(getContent(node));");
	}
}

void xsdSimpleExtension::GenAttrImpl(CppFile & out,Symtab & st)
{
	xsdType::GenAttrImpl(out,st);
}

void xsdSimpleContent::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	if (tashdr() || m_content == NULL)
		return ;
	m_content->GenHeader(out,indent,defaultstr);
}

void xsdComplexRestriction::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	out.iprintln(1,"m_value.sets(getContent(node));");
}

void xsdComplexRestriction::GenWrite(CppFile & out,Symtab & st)
{
	out.iprintln(1,"m_value.Write(out);");
}

void xsdChoice::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	/*
	 * choice parent	    					: group,  choice, sequence, complexType, restriction (simpleContent) , extension(simpleContent),  restriction (complexContent), extension(complexContent)
	 * group parent       					: schema, choice, sequence, complexType, restriction (complexContent), extension (complexContent)
	 * sequence parent    					: group,  choice, sequence, complexType, restriction (simpleContent),  extension (simpleContent), restriction (complexContent), extension (complexContent)
	 * complexType parent 					: element, redefine, schema
	 * restriction (simpleContent) 	: simpleContent
	 * extension(simpleContent)     : simpleContent
	 * restriction (complexContent) : complexContent
	 * extension (complexContent)   : complexContent
	 * simpleContent                : complexType
	 * complexContent								: complexType
	 */
	if (!tashdr())
	{
		bool openstruct = false ;
		if (m_parent->m_tag == type_complex || m_parent->m_tag == type_complexExtension)
		{
			openstruct = true;
			out.iprintln(indent,  "struct %s",getCppName());
			out.iprintln(indent++,"{");
			m_parent->GenAttrHeader(out,indent);
			out.iprintln(indent,"void Parse(xmlNodePtr node);");
			out.iprintln(indent,"void Write(xmlStream & out);");
		}
		out.iprintln(indent,   "struct %s",getTypename());
		out.iprintln(indent++,"{");
		out.iprintln(indent,"void Write(xmlStream & out);");
		/*
		 * generate an enum to determine the taken choice
		 */
		out.iprintln(indent,   "enum xs_enum");
		out.iprintln(indent++,"{");
		out.iprintln(indent,"e_none_selected,");
		for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = * ei ;
			out.iprintln(indent,"%s,",elem->m_choice_selector.c_str());
		}
		out.iprintln(--indent,"} m_selected;\n");

		for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = * ei ;
			out.iprintln(indent,"void set_%s(%s * elem)",elem->getCppName(),elem->m_type->getCppName());
			out.iprintln(indent++,"{");
			out.iprintln(indent,    "m_selected = %s;",elem->m_choice_selector.c_str());
			out.iprintln(indent,    "%s = elem;",elem->getCppName());
			out.iprintln(--indent,"}\n");
		}
		out.iprintln(indent,"bool isset() const { return m_selected != e_none_selected; }\n");
		out.iprintln(indent,"union {");
		m_elements.GenHeader(out,indent+1);
		out.iprintln(indent,"};\n");
		/*
		 * generate constructor to initialize the choices to NULL
		 */
		out.iprintln(indent,"%s()",getTypename());
		out.iprintln(indent++,"{");
		out.iprintln(indent,"m_selected = e_none_selected;");
		for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = * ei ;
			out.iprintln(indent,"%s = NULL;",elem->getCppName());
			break ; // union : einer ist genug
		}
		out.iprintln(--indent,"}\n");

		/*
		 * generate the destructor to delete the selected choice
		 */
		out.iprintln(indent,"~%s()",getTypename());
		out.iprintln(indent++,"{");
		out.iprintln(indent,  "switch(m_selected)");
		out.iprintln(indent++,"{");
		for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = * ei ;
			out.iprintln(indent,"case %s: delete %s; break;",elem->m_choice_selector.c_str(),elem->getCppName());
		}
		out.iprintln(indent,  "default:");
		out.iprintln(indent,  "break;");
		out.iprintln(--indent,"}");

		out.iprintln(--indent,"}");

		out.iprintln(--indent,"} %s;",getVarname());
		if (openstruct)
		{
			out.iprintln(--indent,"};\n");
		}
	}
}

void xsdChoice::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp())
		return ;
	if (m_parent->m_tag == type_complex)
	{
		GenParserChildLoopStart(out,m_elements);
		GenElementCases(out,st,m_elements,true);
		GenParserChildLoopEnd(out);
	}
	else
	{
		GenElementCases(out,st,m_elements,true);
	}
}

void xsdChoice::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_elements.GenLocal(out,st);
}

void xsdChoice::GenWrite(CppFile & out,Symtab & st)
{
	int indent = 1 ;
	out.iprintln(indent,"switch(%s.m_selected)",getVarname());
	out.iprintln(indent++,"{");
	GenWriteElementCases(out,st,m_elements);
	out.iprintln(--indent,"}");
}

void xsdComplexType::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	if (!tashdr())
	{
		if (m_type != NULL)
		{
			m_type->GenHeader(out,indent,defaultstr);
		}
		else
		{
			/*
			 * this type does not have any members
			 */
			out.iprintln(indent,"struct %s",getCppName());
			out.iprintln(indent++,"{");
			m_attributes.GenHeader(out,indent,defaultstr);
			/*
			 * Constructor
			 */
			out.iprintln(indent,"%s()",getCppName());
			out.iprintln(indent++,"{");
			out.iprintln(indent,"m_bset = false;");
			GenInitFixedAttr(out,indent);
			out.iprintln(--indent,"}");

			out.iprintln(indent,"void Parse(xmlNodePtr child);");
			out.iprintln(indent,"void Write(xmlStream & out);");
			if (!m_attributes.empty())
			{
				bool first = true ;
				out.iprintln(indent,"bool isset() const");
				out.iprintln(indent++,"{");
				out.iprintf (indent,"return ");
				for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
				{
					xsdAttribute * a = *ai ;
					if (!first)
					{
						out.printf(" || ");
					}
					out.printf("%s.isset()",a->getCppName());
					first = false;
				}
				out.println(";");
				out.iprintln(--indent,"}");
			}
			else
			{
				out.iprintln(indent,"bool isset() const { return m_bset; }");
			}
			out.iprintln(indent,"void set(bool b)  { m_bset = b; }");
			out.iprintln(indent,"private: bool m_bset;");
			out.iprintln(--indent,"};");
		}
	}
}

void xsdElement::GenHeader(CppFile &out,int indent)
{
	for (typeIterator ti = m_deplist.begin() ; ti != m_deplist.end() ; ti++)
	{
		xsdType * type = *ti;
		if (type->isLocal())
		{
			type->GenHeader(out,indent,getDefault());
		}
	}
	if (m_type == NULL)
	{
		printf("Element %s has unknown type %s\n",getName(),m_typename->m_name.c_str());
		out.iprintln(indent,"%s %s;\n",m_typename->m_name.c_str(),getCppName());
	}
	else
	{
		if (isArray())
		{
			if (isPtr())
				out.iprintln(indent,"xs_array<%s> * %s;",m_type->getCppName(),getCppName());
			else
				out.iprintln(indent,"xs_array<%s> %s;",m_type->getCppName(),getCppName());
		}
		else
		{
			if (isPtr())
				out.iprintln(indent,"%s * %s;",m_type->getCppName(),getCppName());
			else
				out.iprintln(indent,"%s %s;",m_type->getCppName(),getCppName());
		}
	}
}

bool xsdElement::hasAttributes()
{
	bool b = false ;
	if (m_type != NULL)
	{
		b = m_type->hasAttributes();
	}
	return b ;
}

void xsdElement::GenRootHeader(CppFile &out,int indent)
{
	const char * typname = m_type->getCppName();
	out.iprintln(indent,"bool %s_Parse(xmlDocPtr doc, %s & elem);",typname,typname);
	out.iprintln(indent,"void %s_Write(xmlStream & out,%s & elem);",typname,typname);
}

void xsdElement::GenRootImpl(CppFile & out,Symtab & st)
{
	Symbol * s = st.find(getName());
	if (s != NULL)
	{
		const char * typname = m_type->getCppName();
		int indent = 0 ;
		out.iprintln(indent,  "bool %s_Parse(xmlDocPtr doc,%s & elem)",typname,typname);
		out.iprintln(indent++,"{");
		out.iprintln(indent,    "bool res = false;");
		out.iprintln(indent,    "xmlNodePtr child = doc->children;");
		out.iprintln(indent,    "symbols kw = Lookup(child->name);");
		out.iprintln(indent,    "if (kw == sy_%s)",s->m_cname.c_str());
		out.iprintln(indent++,  "{");
		m_type->GenAssignment(out,indent,"elem","getContent(child)");
		out.iprintln(indent,       "res = true;");
		out.iprintln(--indent,  "}");
		out.iprintln(indent,    "return res;");
		out.iprintln(--indent,"}");

		out.iprintln(indent,"void %s_Write(xmlStream & out,%s & elem)",typname,typname);
		out.iprintln(indent++,"{");
		m_cname = "elem";
		GenWrite(out);
		out.iprintln(--indent,"}");
	}
}

void xsdElement::GenLocal(CppFile & out,Symtab & st)
{
	if (m_type != NULL && m_type->isLocal())
	{
		m_type->GenImpl(out,st,getDefault());
	}
}

void xsdElement::GenInit(CppFile & out,int indent)
{
	if (isPtr())
	{
		out.iprintln(indent,"%s = NULL;",getCppName());
	}
}

void xsdElement::GenDelete(CppFile & out,int indent)
{
	if (m_isCyclic)
	{
		out.iprintln(indent,"if (%s != NULL)",getCppName());
		out.iprintln(indent+1,"delete %s;",getCppName());
	}
}

void xsdGroup::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	if (m_type != NULL)
		m_type->GenHeader(out,indent,defaultstr);
}

void xsdGroup::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{

}

void xsdElementList::GenHeader(CppFile & out,int indent)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenHeader(out,indent);
	}
}

#if 0
void xsdElementList::GenImpl(CppFile & out,Symtab & st)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenImpl(out,st);
	}
}
#endif

void xsdElementList::GenLocal(CppFile & out,Symtab & st)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenLocal(out,st);
	}
}

void xsdElementList::GenInit(CppFile & out,int indent)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenInit(out,indent);
	}
}

void xsdElementList::GenDelete(CppFile & out,int indent)
{
	if (hasPointer())
	{
		for (elementIterator ei = begin() ; ei != end() ; ei++)
		{
			xsdElement * elem = *ei ;
			elem->GenDelete(out,indent);
		}
	}
}

bool xsdElementList::hasPointer()
{
	int nptr = 0 ;
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		if (elem->isPtr())
			nptr++;
	}
	return nptr != 0 ;
}

void xsdAttribute::GenHeader(CppFile & out,int indent)
{
	if (m_type != NULL && m_type->isLocal())
	{
		m_type->GenHeader(out,indent,getDefault());
	}
	if (m_type != NULL)
	{
		out.iprintln(indent,"%s %s;",m_type->getCppName(),getCppName());
	}
}

void xsdAttribute::GenLocal(CppFile & out,Symtab & st)
{
	if (m_type != NULL && m_type->isLocal())
	{
		m_type->GenImpl(out,st,getDefault());
	}
}

const char * xsdSimpleRestriction::getReturnType()
{
	return m_base->getCppName() ;
}

void xsdTypeList::GenHeader(CppFile & out,int indent,const char * defaultstr,bool choice)
{
	for (typeIterator ti = begin() ; ti != end() ; ti++)
	{
		(*ti)->GenHeader(out,indent,defaultstr);
	}
}

xsdType* xsdTypeList::Find(const char* name)
{
	assert(false);
	return NULL;
}

void xsdTypeList::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	for (typeIterator ti = begin() ; ti != end() ; ti++)
	{
		xsdType * type = *ti ;
		type->GenImpl(out,st,defaultstr);
	}
}

void xsdTypeList::GenWrite(CppFile & out,Symtab & st)
{
	for (typeIterator ti = begin() ; ti != end() ; ti++)
	{
		xsdType * type = *ti ;
		type->GenWrite(out,st);
	}
}

void xsdType::GenHeader(CppFile & out,int indent,const char * defaultstr)
{

}

void xsdSimpleType::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	if (!tashdr())
	{
		if (m_rest != NULL)
			m_rest->GenHeader(out,indent,defaultstr);
		else if (m_list != NULL)
			m_list->GenHeader(out,indent,defaultstr);
		else if (m_union != NULL)
			m_union->GenHeader(out,indent,defaultstr);
	}
}


void xsdSimpleType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp())
		return ;
	if (m_rest != NULL)
		m_rest->GenImpl(out,st,defaultstr);
	else if (m_list != NULL)
		m_list->GenImpl(out,st,defaultstr);
	else if (m_union != NULL)
		m_union->GenImpl(out,st,defaultstr);
}

void xsdSimpleType::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{

}

void xsdSimpleType::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & leftside,const char * rightside)
{
	if (m_rest != NULL)
		m_rest->GenAssignment(out,indent,leftside,rightside);
	else if (m_list != NULL)
		m_list->GenAssignment(out,indent,leftside,rightside);
	else if (m_union != NULL)
		m_union->GenAssignment(out,indent,leftside,rightside);
}

void xsdSimpleType::setCppName(const char * name)
{
	if (m_rest != NULL)
		m_rest->setCppName(name);
}

void xsdSimpleContent::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp() || m_content == NULL)
		return ;
	m_content->GenImpl(out,st,defaultstr);
}

void xsdSimpleContent::GenAttrImpl(CppFile & out,Symtab & st)
{
	if (m_content == NULL)
		return ;
	m_content->GenAttrImpl(out,st);
}

bool xsdSimpleContent::CheckCycle(xsdElement* elem)
{
	return false;
}

void xsdSimpleContent::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_content != NULL)
		m_content->GenLocal(out,st,defaultstr);
}

void xsdSimpleContent::GenWrite(CppFile & out,Symtab & st)
{
	if (m_content != NULL)
		m_content->GenWrite(out,st);
}

void xsdGroup::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp())
		return ;

}

void xsdSimpleExtension::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_base != NULL)
		m_base->GenLocal(out,st,defaultstr);
}

void xsdSimpleExtension::GenWrite(CppFile & out,Symtab & st)
{
	if (m_base != NULL)
		m_base->GenWrite(out,st);
}

void xsdEnum::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	/*
	 * enums werden in einer struktur gekapselt um die get/set Methoden zu implementieren
	 */
	const char * enumname = getCppName();
	out.iprintln(indent,"enum %s",enumname);
	out.iprintln(indent++,"{");
	out.iprintln(indent,  "%s_invalid,",enumname);
	enumIterator si ;
	for (si = m_values.begin() ; si != m_values.end() ; si++)
	{
		xsdEnumValue * v = *si ;
		out.iprintln(indent,"%s,",v->getEnumValue());
	}
	out.iprintln(--indent,"} ;");

	out.iprintln(indent,"void sets(const char *);");
	out.iprintln(indent,"void set(%s v) { m_value = v; m_bset = true;} ",enumname) ;
	out.iprintln(indent,"const char * gets() const;") ;
	out.iprintln(indent,"%s get() const { return m_value;}",enumname) ;
	for (si = m_values.begin() ; si != m_values.end() ; si++)
	{
		xsdEnumValue * v = *si ;
		out.iprintln(indent,"void set_%s() { set(%s); }",v->getEnumValue(),v->getEnumValue());
	}
	out.iprintln(indent,"bool isset() const { return m_bset;}") ;
	out.iprintln(indent,"private:") ;
	out.iprintln(indent,"%s m_value;",enumname);
	out.iprintln(indent,"bool m_bset;");
	out.iprintln(indent,"public:");
}

void xsdEnum::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	/*
	 * generate code to convert this enum to a string
	 */
	if (tascpp())
		return ;
	std::string qname = m_parent->getQualifiedName();
	out.iprintln(0,"const char * %s::gets() const ",qname.c_str());
	out.iprintln(0,"{");
	  out.iprintln(1,"switch(m_value)");
	  out.iprintln(1,"{");
	  out.iprintln(2,"case %s_invalid: return \"\";",getCppName());
	  for (enumIterator ei = m_values.begin() ; ei != m_values.end() ; ei++)
	  {
		  xsdEnumValue * v = *ei ;
			out.iprintln(2,"case %s: return \"%s\";",v->getEnumValue(),v->m_Value.c_str());
	  }
	  out.iprintln(1,"}");
	  out.iprintln(1,"return \"\";");
	out.iprintln(0,"}\n");

	/*
	 * generate code to convert a string into this enum
	 */
	out.iprintln(0,"void %s::sets(const char * str)",qname.c_str());
	out.iprintln(0,"{");
	  out.iprintln(1,"symbols sy = Lookup(BAD_CAST str);");
	  out.iprintln(1,"switch(sy)");
	  out.iprintln(1,"{");
	  for (enumIterator ei = m_values.begin() ; ei != m_values.end() ; ei++)
	  {
		  xsdEnumValue * val = *ei ;
			out.iprintln(2,"case sy_%s: m_value = %s; m_bset = true ; break;",val->getSymbolName(),val->getEnumValue());
	  }
	    out.iprintln(2,"default: throw xs_invalidString(str,typeid(this).name()) ; break;");
	  out.iprintln(1,"}");
	out.iprintln(0,"}\n");
	out.iprintln(0,"void %s::Write(xmlStream & out)",qname.c_str());
	out.iprintln(0,"{");
	out.iprintln(1,"if (m_bset)");
	out.iprintln(2,"out.putcontent(gets());");
	out.iprintln(0,"}");
}


void xsdSimpleRestriction::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest.getlvalue(),src);
}

enum cmp
{
	cmp_lt, // <
	cmp_le,	// <=
	cmp_eq, // ==
	cmp_ne, // !=
	cmp_gt, // >
	cmp_ge  // >=
};

const char * cmpop(enum cmp op)
{
	switch(op)
	{
		case	cmp_lt : return "<"  ;
		case	cmp_le : return "<=" ;
		case	cmp_eq : return "==" ;
		case	cmp_ne : return "!=" ;
		case	cmp_gt : return ">" ;
		case	cmp_ge : return ">=" ;
	}
	return "--";
}

void GenCheckMinMax(CppFile & out,int indent,enum cmp opmin,enum cmp opmax,bool sign,int minval,int maxval)
{
	if (opmin == cmp_lt && opmax == cmp_gt && minval == maxval)
	{
		/*
		 * x < 1 && x > 1 ergibt x != 1
		 */
		out.iprintln(indent,"if (val %s %d) throw xs_invalidInteger(val,typeid(this).name();",cmpop(cmp_ne),minval);

	}
	if ((sign || minval > 0) && !(opmin == cmp_lt && minval <= 0))
	{
		out.iprintln(indent,"if (val %s %d || val %s %d) throw xs_invalidInteger(val,typeid(this).name());",cmpop(opmin),minval,cmpop(opmax),maxval);
	}
	else
	{
		out.iprintln(indent,"if (val %s %d) throw xs_invalidInteger(val,typeid(this).name());",cmpop(opmax),maxval);
	}
}

void GenCheckMin(CppFile & out,int indent,enum cmp  opmin,bool sign,int minval)
{
	if ((sign || minval > 0) && !(opmin == cmp_lt && minval <= 0))
	{
		out.iprintln(indent,"if (val %s %d) throw xs_invalidInteger(val,typeid(this).name());",cmpop(opmin),minval);
	}
}

void GenCheckMax(CppFile & out,int indent,enum cmp  opmax,bool sign,int maxval)
{
	out.iprintln(indent,"if (val %s %d) throw xs_invalidInteger(val,typeid(this).name());",cmpop(opmax),maxval);
}

void xsdSimpleRestriction::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	const char * tname = getCppName();
	if (m_enum != NULL)
	{
		out.iprintln(indent,"struct %s",tname);
		out.iprintln(indent++,"{");
		m_enum->GenHeader(out,indent,defaultstr);
		/*
		 * generate constructor
		 */
		out.iprintln(indent,"%s()",tname);
		out.iprintln(indent++,"{");
		if (defaultstr != NULL && *defaultstr)
			out.iprintln(indent,"sets(\"%s\");",defaultstr);
		else
		{
			out.iprintln(indent,"m_value = %s_invalid;",m_enum->getCppName());
			out.iprintln(indent,"m_bset = false;");
		}
		out.iprintln(--indent,"}");
		out.iprintln(indent,"void Write(xmlStream & out);");
		out.iprintln(--indent,"};\n");
	}
	else
	{
		if (m_base != NULL)
		{
			const char * basename = m_base->getCppName();
			out.iprintln(indent,"/*");
			out.iprintln(indent," * xs:restriction %s base %s ",getName(),m_base->getCppName());
			out.iprintln(indent," */");
			if (m_base->isString())
				out.iprintln(indent,"struct %s : public xs_string",tname);
			else
				out.iprintln(indent,"struct %s",tname);
			out.iprintln(indent++,"{");
			m_attributes.GenHeader(out,indent,defaultstr);
			if (isScalar() || isString())
			{
				/*
				 * generate constructor
				 */
				if (defaultstr != NULL && *defaultstr)
				{
					out.iprintln(indent,"%s()",tname);
					out.iprintln(indent++,"{");
					out.iprintln(indent,"sets(\"%s\");",defaultstr);
					out.iprintln(--indent,"}");
				}
				if (isString())
				{
					//out.iprintln(indent,"virtual ~%s() {}",tname);
					//out.iprintln(indent,"bool empty() { return m_value.empty();}");
					//out.iprintln(indent,"void sets(const char * str,int n) { xs_string::sets(str,n);}");
					//out.iprintln(indent,"int  length() { return m_value.length();}");
				}
			}
			/*
			 * generate sets function
			 */
			if (m_base->isScalar())
			{
				out.iprintln(indent,  "void sets(const char * str)");
				out.iprintln(indent++,"{");
				out.iprintln(indent,    "%s v;",m_base->getNativeName());
				out.iprintln(indent,    "v = %s::Parse(str);",m_base->getCppName());
				out.iprintln(indent,    "m_value.set(v);");
				out.iprintln(--indent,"}");
			}
			else if (m_base->isString())
			{

				if (m_length.isset() || m_minLength.isset() || m_maxLength.isset())
				{
					out.iprintln(indent,  "void sets(const char *str)");
					out.iprintln(indent++,"{");
					out.iprintln(indent,    "setsn(str,xs_string::mbslen(str));");
					out.iprintln(--indent,"}\n");

					out.iprintln(indent,  "void sets(const char * str,size_t n)");
					out.iprintln(indent++,"{");
					out.iprintln(indent,    "setsn(str,xs_string::mbsnlen(str,n));");
					out.iprintln(--indent,"}\n");

					out.iprintln(indent,  "void setsn(const char *str,size_t len)");
					out.iprintln(indent++,"{");

					if (m_length.isset())
					{
						out.iprintln(indent,"if (len != %d)",m_length.get());
						out.iprintln(indent,"  throw xs_invalidString(str,typeid(this).name());");
					}
					else if (m_minLength.isset() && m_maxLength.isset())
					{
						if (m_minLength.get() == m_maxLength.get())
							out.iprintln(indent,"if (len != %d)",m_minLength.get());
						else if (m_minLength.get() == 0)
							out.iprintln(indent,"if (len > %d)",m_maxLength.get());
						else
							out.iprintln(indent,"if (len < %d || len > %d)",m_minLength.get(),m_maxLength.get());
						out.iprintln(indent,"  throw xs_invalidString(str,typeid(this).name());");
					}
					else if (m_minLength.isset())
					{
						out.iprintln(indent,"if (len < %d)",m_minLength.get());
						out.iprintln(indent,"  throw xs_invalidString(str,typeid(this).name());");
					}
					else if (m_maxLength.isset())
					{
						out.iprintln(indent,"if (len > %d)",m_maxLength.get());
						out.iprintln(indent,"  throw xs_invalidString(str,typeid(this).name());");
					}
					out.iprintln(indent,  "xs_string::sets(str,len);");
					out.iprintln(--indent,"}");
				}
				else
				{
					out.iprintln(indent,  "using xs_string::sets;");
				}
			}
			else
			{
				out.iprintln(indent,  "void sets(const char * str)");
				out.iprintln(indent++,"{");
				out.iprintln(indent,    "m_value.sets(str);");
				out.iprintln(--indent,"}");
			}
			/*
			 * generate set/get function
			 */
			if (!m_base->isString())
			{
				out.iprintln(indent,   "void set(const %s & val)",getReturnType());
				out.iprintln(indent++,"{");
				out.iprintln(indent,     "m_value = val;");
				out.iprintln(--indent,"}");
			}
			if (m_base->isScalar())
			{
				bool sign = m_base->isSigned() ;
				out.iprintln(indent,   "void set(%s val)",m_base->getNativeName());
				out.iprintln(indent++,"{");
				if      (m_minExclusive.isset() && m_maxExclusive.isset())
				{
					GenCheckMinMax(out,indent,cmp_le,cmp_ge,sign,m_minExclusive.get(),m_maxExclusive.get());
				}
				else if (m_minExclusive.isset() && m_maxInclusive.isset())
					GenCheckMinMax(out,indent,cmp_le,cmp_gt,sign,m_minExclusive.get(),m_maxInclusive.get());
				else if (m_minInclusive.isset() && m_maxExclusive.isset())
					GenCheckMinMax(out,indent,cmp_lt,cmp_ge,sign,m_minInclusive.get(),m_maxExclusive.get());
				else if (m_minInclusive.isset() && m_maxInclusive.isset())
					GenCheckMinMax(out,indent,cmp_lt,cmp_gt,sign,m_minInclusive.get(),m_maxInclusive.get());
				else
				{
					if (m_minExclusive.isset())
						GenCheckMin(out,indent,cmp_le,sign,m_minExclusive.get());
					else if (m_maxExclusive.isset())
						GenCheckMax(out,indent,cmp_ge,sign,m_maxExclusive.get());
					else if (m_minInclusive.isset())
						GenCheckMin(out,indent,cmp_lt,sign,m_minInclusive.get());
					else if (m_maxInclusive.isset())
						GenCheckMax(out,indent,cmp_gt,sign,m_maxInclusive.get());
				}
				out.iprintln(indent,    "m_value.set(val);");
				out.iprintln(--indent,"}");
			}
			if (!m_base->isString())
			{
				/*
				out.iprintln(indent,  "const %s & get() const",getReturnType());
				out.iprintln(indent++,"{");
				out.iprintln(indent,"  return m_value;");
				out.iprintln(--indent,"}");
				*/
			}
			/*
			 * generate the gets function
			 */
			if (m_base->isString())
			{
/*
				out.iprintln(indent,"const char * gets() const");
				out.iprintln(indent++,"{");
				out.iprintln(indent,"return m_value.gets();");
				out.iprintln(--indent,"}");
				*/
			}
			else
			{
				out.iprintln(indent,"xs_string gets() const ");
				out.iprintln(indent++,"{");
				out.iprintln(indent,"return m_value.gets();");
				out.iprintln(--indent,"}");

				//out.iprintln(indent,"void validate() throw();");
			}
			if (!m_base->isString())
			{
				out.iprintln(indent,"bool isset() const { return m_value.isset();}") ;
				out.iprintln(indent,"void Write(xmlStream & out);");
				out.iprintln(indent,"%s m_value;",basename);
			}
			out.iprintln(--indent,"};\n");
		}
		else if (m_simple != NULL)
		{
			m_simple->GenHeader(out,indent,defaultstr);
		}
	}
}

void xsdSimpleRestriction::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
#if DEBUG
	printf("%s %s\n",__FUNCTION__,getCppName());
#endif
	if (tascpp())
		return ;
	if (m_enum != NULL)
	{
		m_enum->GenImpl(out,st,defaultstr);
	}
	else if (m_base != NULL)
	{
		if (!m_base->isString())
		{
			m_base->GenImpl(out,st,defaultstr);
			out.iprintln(0,"void %s::Write(xmlStream & out)",getQualifiedName().c_str());
			out.iprintln(0,"{");
			out.iprintln(1,"if (m_value.isset())");
			out.iprintln(2,"m_value.Write(out);");
			out.iprintln(0,"}");
		}
	}
	else if (m_simple != NULL)
	{
		m_simple->GenImpl(out,st,defaultstr);
	}
	else
	{
	}
}

void xsdSimpleRestriction::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{

}

void xsdSimpleRestriction::setCppName(const char * name)
{
	if (m_enum != NULL)
		m_enum->setCppName(name);
}

bool xsdSimpleRestriction::isString()
{
	return m_base->isString() ;
}

bool xsdSimpleRestriction::isStruct()
{
	if (m_enum != NULL)
		return true;
	if (m_base != NULL)
		return m_base->isStruct();
	if (m_simple != NULL)
		return m_simple->isStruct();
	return false ;
}

bool xsdSimpleRestriction::isScalar()
{
	bool res = false ;
	res = m_base->isInteger() || m_base->isfloat();
	return res ;
}

void xsdList::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	if (!tashdr())
	{
		if (m_itemtype != NULL)
		{
			out.iprintln(indent,"struct %s",getCppName());
			out.iprintln(indent++,"{");
			if (m_itemtype->isLocal())
			{
				m_itemtype->m_cname = "item_t";
			}
			m_itemtype->GenHeader(out,indent,defaultstr);
			if (defaultstr != NULL && *defaultstr != 0)
			{

				out.iprintln(indent,"%s()",getCppName());
				out.iprintln(indent++,"{");
				out.iprintln(indent,"sets(\"%s\");",defaultstr);
				out.iprintln(--indent,"}");
			}
			out.iprintln(indent,  "void sets(const char * str)");
			out.iprintln(indent++,"{");
			out.iprintln(indent,  "m_list.sets(str);");
			out.iprintln(--indent,"}");

			out.iprintln(indent,  "int count() const");
			out.iprintln(indent++,"{");
			out.iprintln(indent,  "return m_list.count();");
			out.iprintln(--indent,"}");

			out.iprintln(indent,  "const %s & getAt(int pos) const",m_itemtype->getCppName());
			out.iprintln(indent++,"{");
			out.iprintln(indent,  "return m_list.getAt(pos);");
			out.iprintln(--indent,"}");

			out.iprintln(indent,  "void setAt(int pos,const %s & val)",m_itemtype->getCppName());
			out.iprintln(indent++,"{");
			out.iprintln(indent,  "m_list.setAt(pos,val);");
			out.iprintln(--indent,"}");

			out.iprintln(indent,  "xs_string gets() const");
			out.iprintln(indent++,"{");
			out.iprintln(indent,  "return m_list.gets();");
			out.iprintln(--indent,"}");
			out.iprintln(indent,"bool isset() const { return m_bset;}") ;
			out.iprintln(indent,"xs_list<%s> m_list;",m_itemtype->getCppName());
			out.iprintln(indent,"bool m_bset;");
			out.iprintln(--indent,"};\n");
		}
	}
}

void xsdList::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (tascpp())
		return ;
	if (m_itemtype == NULL)
		m_itemtype = FindType(m_itemtypename);
	if (m_itemtype != NULL)
	{
		if (m_itemtype->isLocal())
			m_itemtype->GenImpl(out,st,defaultstr);
	}
}

void xsdList::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{

}

void xsdList::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest.getlvalue(),src);
}

void xsdList::setCppName(const char * name)
{
	m_parent->setCppName(name);
}

void xsdType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{

}

void xsdType::GenWrite(CppFile & out,Symtab & st)
{
	const char * name = getCppName();
	out.iprintln(1,"if (%s::isset()) %s::Write(out);\n",name,name);
}

void xsdType::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest.getlvalue(),src);
}

void xsdType::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
  out.iprintln(indent,"%s.sets(%s);",dest,src);
}

void xsdComplexType::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_type != NULL)
		m_type->GenLocal(out,st,defaultstr);
}

void GenParserAttrLoop(CppFile & out,Symtab & st,xsdAttrList & attributes)
{
	if (!attributes.empty())
	{
		out.iprintln(1,"for_each_attr(attr,node)");
		out.iprintln(1,"{");
		out.iprintln(2,"symbols kw;");
		out.iprintln(2,"kw=Lookup(attr->name);");
		out.iprintln(2,"switch(kw)");
		out.iprintln(2,"{");
		for (attrIterator ai = attributes.begin() ; ai != attributes.end() ; ai++)
		{
			xsdAttribute * a = *ai ;
			Symbol * s = st.find(a->m_name.c_str());
			if (s != NULL)
			{
				out.iprintln(3,"case sy_%s:",s->m_cname.c_str());
				a->m_type->GenAssignment(out,4,*a,"getContent(attr)");
				out.iprintln(3,"break;");
			}
		}
		out.iprintln(3,"default:");
		out.iprintln(3,"break;");
		out.iprintln(2,"}");
		out.iprintln(1,"}");
  }
}

void xsdAttribute::GenWrite(CppFile & out,int indent,xsdElement * elem)
{
	std::string var = elem->getlvalue();
	if (elem->isArray())
		var += "[i]";
	if (m_type != NULL && m_type->isScalar())
	{
		const char * getfnc = "get";

		if (m_type->m_tag == type_boolean)
			getfnc = "gets";
		if (elem->isChoice())
		{
			out.iprintln(indent++,"if (sel->%s.isset())",getCppName());
			out.iprintln(indent,"out.putattr(\"%s\",sel->%s.%s());",getName(),getCppName(),getfnc);
		}
		else
		{
			out.iprintln(indent++,"if (%s.%s.isset())",var.c_str(),getCppName());
			out.iprintln(indent,"out.putattr(\"%s\",%s.%s.%s());",getName(),var.c_str(),getCppName(),getfnc);
		}
	}
	else
	{
		if (elem->isChoice())
		{
			out.iprintln(indent++,"if (sel->%s.isset())",getCppName());
			out.iprintln(indent,"out.putattr(\"%s\",sel->%s.gets());",getName(),getCppName());
		}
		else
		{
			out.iprintln(indent++,"if (%s.%s.isset())",var.c_str(),getCppName());
			out.iprintln(indent,"out.putattr(\"%s\",%s.%s.gets());",getName(),var.c_str(),getCppName());
		}
	}
}

void xsdAttrList::GenWrite(CppFile & out,int indent,xsdElement * elem)
{
	for (attrIterator ai = begin() ; ai != end() ; ai++)
	{
		xsdAttribute * attr = *ai;
		attr->GenWrite(out,indent,elem);
	}
}

void xsdAttrList::GenLocal(CppFile & out,Symtab & st)
{
	for (attrIterator ai = begin() ; ai != end() ; ai++)
	{
		xsdAttribute * attr = *ai ;
		attr->GenLocal(out,st);
	}
}

void xsdComplexContent::GenHeader(CppFile& out, int indent,const char* defaultstr)
{
	if (!tashdr())
	{
		if (m_type != NULL)
			m_type->GenHeader(out,indent,defaultstr);
	}
}

void xsdComplexContent::GenImpl(CppFile& out, Symtab& st,const char* defaultstr)
{
	if (tascpp() || m_type == NULL)
		return ;
	m_type->GenImpl(out,st,defaultstr);
}

void xsdComplexContent::GenLocal(CppFile& out, Symtab& st,const char* defaultstr)
{
	if (m_type != NULL)
	{
		m_type->GenLocal(out,st,defaultstr);
	}
}

void xsdComplexType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	int i = 0 ;
	if (tascpp())
		return ;
	if (m_type != NULL)
		m_type->GenLocal(out,st,defaultstr);
	out.iprintln(i,"/*\n * complexType %s\n*/",getName());
	GenAttrImpl(out,st);
	out.iprintln(i,"void %s::Parse(xmlNodePtr node)",getQualifiedName().c_str());
	out.iprintln(i,"{");
	if (hasAttributes())
	{
		xsdAttrList alst ;
		GetAttributes(alst);
		GenParserAttrLoop(out,st,alst);
	}
	if (m_type != NULL)
	{
		m_type->GenImpl(out,st,defaultstr);
	}
	else
	{
		out.iprintln(i+1,"m_bset = true;");
	}
	out.iprintln(i,"}");
	out.println();
	out.iprintln(i,"void %s::Write(xmlStream & out)",getQualifiedName().c_str());
	out.iprintln(i++,"{");
	if (m_type != NULL)
		m_type->GenWrite(out,st);
	out.iprintln(--i,"}");
}

void xsdComplexType::GenAttrImpl(CppFile & out,Symtab & st)
{
	xsdType::GenAttrImpl(out,st);
	if (m_type != NULL)
		m_type->GenAttrImpl(out,st);
}

void xsdComplexType::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	if (dest.isPtr())
	{
		if (dest.isChoice())
			out.iprintln(indent,"%s.%s = new %s;",dest.getChoiceVarname(),dest.getCppName(),dest.m_type->getCppName());
		else
			out.iprintln(indent,"%s = new %s;",dest.getCppName(),dest.m_type->getCppName());
	}
	out.iprintln(indent,"%s.Parse(child);",dest.getlvalue());
}

void xsdComplexType::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
	out.iprintln(indent,"%s.Parse(child);",dest);
}

bool xsdEnum::CheckCycle(xsdElement* elem)
{
	return false;
}


void xsdComplexExtension::CalcDependency(xsdTypeList & list)
{
	if (m_base == NULL)
	{
		m_base = FindType(m_basetypename);
	}
	if (m_base != NULL && !m_base->tascd())
	{
		list.push_back(m_base);
	}
	if (m_type != NULL)
		m_type->CalcDependency(list);
}

void xsdComplexExtension::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	if (m_type != NULL)
		m_type->GenHeader(out,indent,defaultstr);
	else
	{
		xsdType * p = m_parent;
		if (p != NULL && p->m_parent != NULL)
		{
			p = p->m_parent;
			if (p->m_tag == type_complex)
			{
				if (m_base != NULL)
					out.iprintln(indent,"struct %s : public %s",p->getCppName(),m_base->getCppName());
				else
					out.iprintln(indent,"struct %s",getCppName());
				out.iprintln(indent++,"{");
				if (HasFixedAttributes())
				{
					out.iprintln(indent,"%s()",getCppName());
					out.iprintln(indent,"{");
					GenInitFixedAttr(out,indent+1);
					out.iprintln(indent,"}");
				}
				GenAttrHeader(out,indent);
				out.iprintln(indent,"void Parse(xmlNodePtr node);");
				out.iprintln(indent,"void Write(xmlStream & out);");
				out.iprintln(--indent,"};");
			}
		}
	}
}

void xsdComplexExtension::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_type != NULL)
		m_type->GenImpl(out,st,defaultstr);
	else
	{
		out.iprintln(1,"%s::Parse(node);",m_basetypename->getCppName());
	}
}

void xsdComplexExtension::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_type != NULL)
		m_type->GenLocal(out,st,defaultstr);
}

