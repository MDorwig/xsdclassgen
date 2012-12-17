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
				const char * choicename = elem->getChoiceName();
				out.iprintln(id++,"{");
				out.iprintln(id,"%s.m_selected = xs_choice::%s;",choicename,elem->m_choice_selector.c_str());;
				if (elem->isArray())
				{
					out.iprintln(id,"%s tmp;",elem->m_type->getCppName());
					elem->m_type->GenAssignment(out,id,"tmp","getContent(child)");
					if (elem->isPtr())
					{
						out.iprintln(id,"if (%s.%s == NULL)",choicename,elem->getCppName());
						out.iprintln(id+1,"%s.%s = new xs_array<%s>;",choicename,elem->getCppName(),elem->m_type->getCppName());
						out.iprintln(id,"%s.%s->add(tmp);",choicename,elem->getCppName());
					}
					else
						out.iprintln(id,"%s.%s.add(tmp);",choicename,elem->getCppName());
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
					out.iprintln(indent,"%s_present = true;",elem->getCppName());
					if (elem->isArray())
					{
						if (!elem->isUnbounded())
							out.iprintln(indent,"if (%s < %d)",elem->getIndexVar(),elem->m_maxOccurs);
						out.iprintln(indent++,"{");
						out.iprintln(indent,  "%s tmp;",typname);
						elem->m_type->GenAssignment(out,indent,"tmp","getContent(child)");
						if (elem->isPtr())
							out.iprintln(indent,"%s->add(tmp);",elem->getCppName());
						else
							out.iprintln(indent,"%s.add(tmp);",elem->getCppName());
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


void GenParserChildLoopStart(CppFile & out,Symtab & st,xsdElementList & elements,const char * defaultstr,bool ischoice)
{
	for (elementIterator ei = elements.begin() ; ei != elements.end() ; ei++)
	{
		xsdElement * e = *ei ;
		if (e->isArray() && !e->isUnbounded())
			out.iprintln(1,"int %s = 0;",e->getIndexVar());
	}
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

void xsdExtension::GenHeader(CppFile &out,int indent,const char * defaultstr,bool isroot)
{
	out.iprintln(indent,"struct %s : public",getCppName());
	out.iprintln(indent++,"{");
	if (isroot)
		out.iprintln(indent,"void Parse(xmlDocPtr node);");
	else
		out.iprintln(indent,"void Parse(xmlNodePtr node);");
	m_attributes.GenHeader(out,indent,defaultstr);
	out.iprintln(indent,"%s %s;\n",m_basetypename->getCppName(),"m_val");
	out.iprintln(--indent,"};");
}

void xsdExtension::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp())
		return ;
}


void xsdSimpleContent::GenHeader(CppFile &out,int indent,const char * defaultstr,bool isroot)
{
	if (tashdr() || m_content == NULL)
		return ;
	m_content->GenHeader(out,indent,defaultstr,isroot);
}

void xsdChoice::GenHeader(CppFile &out,int indent,const char * defaultstr,bool isroot)
{
	if (!tashdr())
	{
		if (m_parent->m_tag == type_complex)
		{
			out.iprintln(indent,  "struct %s",getCppName());
			out.iprintln(indent++,"{");
			m_parent->GenAttrHeader(out,indent);
			if (isroot)
				out.iprintln(indent,"void Parse(xmlDocPtr node);");
			else
				out.iprintln(indent,"void Parse(xmlNodePtr node);");
		}
		const char * choicename = "xs_choice";
		out.iprintln(indent,   "struct %s",choicename);
		out.iprintln(indent++,"{");
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
		out.iprintln(--indent,"} m_selected;");

		m_elements.GenHeader(out,indent);
		/*
		 * generate constructor to initialize the choices to NULL
		 */
		out.iprintln(indent,"%s()",choicename);
		out.iprintln(indent++,"{");
		out.iprintln(indent,"m_selected = e_none_selected;");
		for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = * ei ;
			out.iprintln(indent,"%s = NULL;",elem->getCppName());
		}
		out.iprintln(--indent,"}");

		/*
		 * generate the destructor to delete the selected choice
		 */
		out.iprintln(indent,"~%s()",choicename);
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

		out.iprintln(--indent,"} m_choice;");
	}
	if (m_parent->m_tag == type_complex)
	{
		out.iprintln(--indent,"};");
	}
}

void xsdChoice::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp())
		return ;
	if (m_parent->m_tag == type_complex)
	{
		GenParserChildLoopStart(out,st,m_elements,defaultstr,true);
		GenElementCases(out,st,m_elements,true);
		GenParserChildLoopEnd(out);
	}
	else
	{
		GenElementCases(out,st,m_elements,true);
	}
}

void xsdChoice::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	m_elements.GenLocal(out,st);
}


void xsdComplexType::GenAttrHeader(CppFile & out,int indent)
{
	if (!m_attributes.empty())
	{
		m_attributes.GenHeader(out,indent,"");
	}
}

void xsdComplexType::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
{
	if (!tashdr())
	{
		if (m_type != NULL)
		{
			m_type->GenHeader(out,indent,defaultstr,isroot);
		}
		else if (!m_attributes.empty())
		{
			out.iprintln(indent,"struct %s",getCppName());
			out.iprintln(indent++,"{");
			m_attributes.GenHeader(out,indent,defaultstr);
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
			type->GenHeader(out,indent,getDefault(),false);
		}
	}
	if (m_type == NULL)
	{
		printf("Element %s has unknown type %s\n",getName(),m_typename->m_name.c_str());
		out.iprintln(indent,"%s %s;\n",m_typename->m_name.c_str(),getCppName());
	}
	else
	{
		if (!m_isChoice)
			out.iprintln(indent,"bool %s_present;",getCppName());
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


void xsdElement::GenImpl(CppFile & out,Symtab & st)
{
	for (typeIterator ti = m_deplist.begin() ; ti != m_deplist.end() ; ti++)
	{
		xsdType * t = *ti ;
		t->GenImpl(out,st,getDefault(),false);
	}
}

void xsdElement::GenLocal(CppFile & out,Symtab & st)
{
	if (m_type != NULL && m_type->isLocal())
	{
		m_type->GenImpl(out,st,getDefault(),false);
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

void xsdGroup::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
{
	if (m_type != NULL)
		m_type->GenHeader(out,indent,defaultstr,isroot);
}

void xsdGroup::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
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

void xsdElementList::GenImpl(CppFile & out,Symtab & st)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenImpl(out,st);
	}
}

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
		if (elem->m_isCyclic)
			nptr++;
	}
	return nptr != 0 ;
}

void xsdAttribute::GenHeader(CppFile & out,int indent)
{
	if (m_type != NULL)
	{
		out.iprintln(indent,"%s %s;",m_type->getCppName(),getCppName());
	}
}

const char * xsdRestriction::getReturnType()
{
	return m_base->getCppName() ;
}

void xsdTypeList::GenHeader(CppFile & out,int indent,const char * defaultstr,bool choice)
{
	for (typeIterator ti = begin() ; ti != end() ; ti++)
	{
		(*ti)->GenHeader(out,indent,defaultstr,false);
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
		type->GenImpl(out,st,defaultstr,false);
	}
}

void xsdType::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
{

}

void xsdSimpleType::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
{
	if (!tashdr())
	{
		if (m_rest != NULL)
			m_rest->GenHeader(out,indent,defaultstr,isroot);
		else if (m_list != NULL)
			m_list->GenHeader(out,indent,defaultstr,isroot);
		else if (m_union != NULL)
			m_union->GenHeader(out,indent,defaultstr,isroot);
	}
}


void xsdSimpleType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp())
		return ;
	if (m_rest != NULL)
		m_rest->GenImpl(out,st,defaultstr,isroot);
	else if (m_list != NULL)
		m_list->GenImpl(out,st,defaultstr,isroot);
	else if (m_union != NULL)
		m_union->GenImpl(out,st,defaultstr,isroot);
}

void xsdSimpleType::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
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

void xsdSimpleContent::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp() || m_content == NULL)
		return ;
	m_content->GenImpl(out,st,defaultstr,isroot);
}

bool xsdSimpleContent::CheckCycle(xsdElement* elem)
{
	return false;
}

void xsdSimpleContent::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
}

void xsdGroup::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp())
		return ;

}

void xsdExtension::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{

}

void xsdEnum::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
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
	out.iprintln(indent,"void set(%s v) { m_value = v;} ",enumname) ;
	out.iprintln(indent,"const char * gets() const;") ;
	out.iprintln(indent,"%s get() const { return m_value;}",enumname) ;
	out.iprintln(indent,"%s m_value;",enumname);
}

void xsdEnum::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
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
			out.iprintln(2,"case sy_%s: m_value = %s; break;",val->getSymbolName(),val->getEnumValue());
	  }
	    out.iprintln(2,"default: m_value = %s_invalid; break;",getCppName());
	  out.iprintln(1,"}");
	out.iprintln(0,"}\n");
}


void xsdRestriction::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest.getlvalue(),src);
}

void xsdRestriction::GenHeader(CppFile & out,int indent,const char * defaultstr,bool isroot)
{
	const char * tname = getCppName();
	if (m_enum != NULL)
	{
		out.iprintln(indent,"struct %s",tname);
		out.iprintln(indent++,"{");
		m_enum->GenHeader(out,indent,defaultstr,isroot);
		/*
		 * generate constructor
		 */
		out.iprintln(indent,"%s()",tname);
		out.iprintln(indent++,"{");
		if (defaultstr != NULL && *defaultstr)
			out.iprintln(indent,"sets(\"%s\");",defaultstr);
		else
			out.iprintln(indent,"m_value = %s_invalid;",m_enum->getCppName());
		out.iprintln(--indent,"}");
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
			out.iprintln(indent,"struct %s",tname);
			out.iprintln(indent++,"{");
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
			}
			/*
			 * generate sets function
			 */
			out.iprintln(indent,  "void sets(const char * str)");
			out.iprintln(indent++,"{");
			out.iprintln(indent,  "m_value.sets(str);");
			out.iprintln(--indent,"}");
			/*
			 * generate set/get function
			 */
			out.iprintln(indent,"void set(const %s & val)",getReturnType());
			out.iprintln(indent++,"{");
			out.iprintln(indent,"m_value = val;");
			out.iprintln(--indent,"}");

			if (!m_base->isString())
			{
				out.iprintln(indent,"const %s & get() const",getReturnType());
				out.iprintln(indent++,"{");
				out.iprintln(indent,"  return m_value;");
				out.iprintln(--indent,"}");
			}
			/*
			 * generate the gets function
			 */
			if (m_base->isString())
			{
				out.iprintln(indent,"const char * gets() const");
				out.iprintln(indent++,"{");
				out.iprintln(indent,"return m_value.gets();");
				out.iprintln(--indent,"}");
			}
			else
			{
				out.iprintln(indent,"xs_string gets() const ");
				out.iprintln(indent++,"{");
				out.iprintln(indent,"return m_value.gets();");
				out.iprintln(--indent,"}");

				//out.iprintln(indent,"void validate() throw();");
			}
			out.iprintln(indent,"%s m_value;",basename);
			out.iprintln(--indent,"};\n");
		}
		else if (m_simple != NULL)
		{
			m_simple->GenHeader(out,indent,defaultstr,isroot);
		}
	}
}

void xsdRestriction::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
#if DEBUG
	printf("%s %s\n",__FUNCTION__,getCppName());
#endif
	if (tascpp())
		return ;
	if (m_enum != NULL)
	{
		m_enum->GenImpl(out,st,defaultstr,isroot);
	}
	else if (m_simple != NULL)
	{
		m_simple->GenImpl(out,st,defaultstr,isroot);
	}
	else
	{
	}
}

void xsdRestriction::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{

}

void xsdRestriction::setCppName(const char * name)
{
	if (m_enum != NULL)
		m_enum->setCppName(name);
}

bool xsdRestriction::isString()
{
	return m_base->isString() ;
}

bool xsdRestriction::isScalar()
{
	bool res = false ;
	res = m_base->isInteger() || m_base->isfloat();
	return res ;
}

void xsdList::GenHeader(CppFile &out,int indent,const char * defaultstr,bool isroot)
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
			m_itemtype->GenHeader(out,indent,defaultstr,isroot);
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

			out.iprintln(indent,"xs_list<%s> m_list;",m_itemtype->getCppName());
			out.iprintln(--indent,"};\n");
		}
	}
}

void xsdList::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp())
		return ;
	if (m_itemtype == NULL)
		m_itemtype = FindType(m_itemtypename);
	if (m_itemtype != NULL)
	{
		if (m_itemtype->isLocal())
			m_itemtype->GenImpl(out,st,defaultstr,isroot);
	}
}

void xsdList::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{

}

void xsdList::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	out.iprintln(indent,"%ssets(%s);",dest.getlvalue(),src);
}

void xsdList::setCppName(const char * name)
{
	m_parent->setCppName(name);
}

void xsdType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{

}

void xsdType::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest.getlvalue(),src);
}

void xsdType::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
  out.iprintln(indent,"%s.sets(%s);",dest,src);
}

void xsdComplexType::GenLocal(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (m_type != NULL)
		m_type->GenLocal(out,st,defaultstr,isroot);
}

void GenParserAttrLoop(CppFile & out,Symtab & st,xsdAttrList & attributes,const char * defaultstr)
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

void xsdComplexType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr,bool isroot)
{
	if (tascpp() || m_type == NULL)
		return ;
	m_type->GenLocal(out,st,defaultstr,isroot);
	out.iprintln(0,"/*\n * complexType %s\n*/",getName());
	if (isroot)
		out.iprintln(0,"void %s::Parse(xmlDocPtr node)",getQualifiedName().c_str());
	else
		out.iprintln(0,"void %s::Parse(xmlNodePtr node)",getQualifiedName().c_str());
	out.iprintln(0,"{");
	GenParserAttrLoop(out,st,m_attributes,defaultstr);
	if (m_type != NULL)
	{
		m_type->GenImpl(out,st,defaultstr,false);
	}
	out.iprintln(0,"}");
	out.println();
}

void xsdComplexType::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	if (dest.isPtr())
	{
		if (dest.isChoice())
			out.iprintln(indent,"%s.%s = new %s;",dest.getChoiceName(),dest.getCppName(),dest.m_type->getCppName());
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



