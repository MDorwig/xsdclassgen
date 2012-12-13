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
#include <libxml2/libxml/parser.h>
#include "xsdclassgen.h"

void xsdSequence::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	std::list<xsdElement*>::iterator ei;
	out.iprintln(indent,"struct %s",getCppName());
	out.iprintln(indent++,"{");
	/*
	 * Constructor
	 */
	out.iprintln(indent,"%s()",getCppName());
	out.iprintln(indent++,"{");
	m_elements.GenInit(out,indent);
	out.iprintln(--indent,"}");
	/*
	 * Parse Function
	 */
	out.iprintln(indent,"void Parse(xmlNodePtr node);");
	m_parent->GenAttrHeader(out,indent);
	m_elements.GenHeader(out,indent);
	m_types.GenHeader(out,indent,defaultstr,false);
	out.iprintln(--indent,"};\n");
}

void xsdAll::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	std::list<xsdElement*>::iterator ei;
	out.iprintln(indent,"struct %s",getCppName());
	out.iprintln(indent++,"{");
	/*
	 * Constructor
	 */
	out.iprintln(indent,"%s()",getCppName());
	out.iprintln(indent++,"{");
	m_elements.GenInit(out,indent);
	out.iprintln(--indent,"}");

	out.iprintln(indent,"void Parse(xmlNodePtr node);");
	m_parent->GenAttrHeader(out,indent);
	m_elements.GenHeader(out,indent);
	out.iprintln(--indent,"};\n");
}

void xsdExtension::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	out.iprintln(indent,"struct %s",getCppName());
	out.iprintln(indent++,"{");
	for (xsdAttrList::iterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
	{
		xsdAttribute * attr = *ai ;
		attr->GenHeader(out,indent);
	}
	out.iprintln(indent,"%s %s;\n",m_basetypename->getCppName(),"m_val");
	out.iprintln(--indent,"};\n");
}

void xsdSimpleContent::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	if (!m_hdrimpl)
	{
		m_hdrimpl = true;
		if (m_content != NULL)
		{
			m_content->GenHeader(out,indent,defaultstr);
		}
	}
}


void xsdChoice::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	if (!m_hdrimpl)
	{
		m_hdrimpl = true ;
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

		out.iprintln(--indent,"} m_%s;",choicename);
	}
}

void xsdComplexType::GenAttrHeader(CppFile & out,int indent)
{
	if (!m_attributes.empty())
	{
		for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
		{
			xsdAttribute * attr = *ai;
			attr->GenHeader(out,indent);
		}
	}
}

void xsdComplexType::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	if (!m_hdrimpl)
	{
		m_hdrimpl = true;
		if (m_type != NULL)
		{
			m_type->GenHeader(out,indent,defaultstr);
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


void xsdElement::GenImpl(CppFile & out,Symtab & st)
{
	for (typeIterator ti = m_deplist.begin() ; ti != m_deplist.end() ; ti++)
	{
		xsdType * t = *ti ;
		t->GenImpl(out,st,getDefault());
	}
}

void xsdElement::GenLocal(CppFile & out,Symtab & st)
{
	if (m_type->isLocal())
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

void xsdSimpleType::GenHeader(CppFile & out,int indent,const char * defaultstr)
{
	if (!m_hdrimpl)
	{
		m_hdrimpl = true;
		if (m_rest != NULL)
			m_rest->GenHeader(out,indent,defaultstr);
		else if (m_list != NULL)
			m_list->GenHeader(out,indent,defaultstr);
		else if (m_union != NULL)
			m_union->GenHeader(out,indent,defaultstr);
	}
}

void xsdType::GenHeader(CppFile & out,int indent,const char * defaultstr)
{

}

void GenParserChildLoop(CppFile & out,Symtab & st,xsdElementList & elements,xsdTypeList & types,const char * defaultstr)
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
	for (elementIterator ei = elements.begin() ; ei != elements.end() ; ei++)
	{
		xsdElement * e = *ei ;
		Symbol * s = st.find(e->m_name.c_str());
		if (s != NULL)
		{
			out.iprintln(4,"case sy_%s:",s->m_cname.c_str());
			if (e->m_type != NULL)
			{
				int indent = 5 ;
				const char * typname = e->m_type->getCppName();
				if (e->isArray())
				{
					if (!e->isUnbounded())
						out.iprintln(indent,"if (%s < %d)",e->getIndexVar(),e->m_maxOccurs);
					out.iprintln(indent++,"{");
					out.iprintln(indent,  "%s tmp;",typname);
					e->m_type->GenAssignment(out,indent,"tmp","getContent(child)");
					if (e->isPtr())
						out.iprintln(indent,"%s->add(tmp);",e->getCppName());
					else
						out.iprintln(indent,"%s.add(tmp);",e->getCppName());
					out.iprintln(--indent,"}");
				}
				else
				{
					e->m_type->GenAssignment(out,indent,*e,"getContent(child)");
				}
			}
			out.iprintln(4,"break;");
		}
	}
	for (typeIterator ti = types.begin() ; ti != types.end() ; ti++)
	{
		xsdType * type = *ti ;
		type->GenImpl(out,st,defaultstr);
	}
	out.iprintln(4,"default:");
	out.iprintln(4,"break;");
	out.iprintln(3,"}");
	out.iprintln(2,"}");
	out.iprintln(1,"}");
}

void xsdAll::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	xsdTypeList lst ;
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
	GenParserChildLoop(out,st,m_elements,lst,defaultstr);
}

void xsdAll::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_elements.GenLocal(out,st);
}


void xsdSequence::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
	GenParserChildLoop(out,st,m_elements,m_types,defaultstr);
}

void xsdSequence::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_elements.GenLocal(out,st);
	//m_types.GenImpl(out,st,defaultstr);
}

void xsdChoice::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
	for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		Symbol * s = st.find(elem->m_name.c_str());
		if (s != NULL)
		{
			int id = 4 ;
			out.iprintln(id,"case sy_%s:",s->m_cname.c_str());
			out.iprintln(id++,"{");
			out.iprintln(id,"m_choice.m_selected = xs_choice::%s;",elem->m_choice_selector.c_str());;
			if (elem->isArray())
			{
				out.iprintln(id,"%s tmp;",elem->m_type->getCppName());
				elem->m_type->GenAssignment(out,id,"tmp","getContent(child)");
				if (elem->isPtr())
				{
					out.iprintln(id,"if (m_choice.%s == NULL)",elem->getCppName());
					out.iprintln(id+1,"m_choice.%s = new xs_array<%s>;",elem->getCppName(),elem->m_type->getCppName());
					out.iprintln(id,"m_choice.%s->add(tmp);",elem->getCppName());
				}
				else
					out.iprintln(id,"%m_choice.%s.add(tmp);",elem->getCppName());
			}
			else
			{
				out.iprintln(id,"m_choice.%s = new %s;",elem->getCppName(),elem->m_type->getCppName());
				elem->m_type->GenAssignment(out,id,*elem,"getContent(child)");
			}
			out.iprintln(--id,"}");
			out.iprintln(id,"break;");
		}
	}
}

void xsdChoice::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_elements.GenLocal(out,st);
}

int xsdSimpleType::getDim()
{
	int n = 1 ;
	if (m_rest != NULL)
		n = m_rest->getDim();
	else
		n = m_parent->getDim();
	return n ;
}

void xsdSimpleType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
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
	if (m_cppimpl)
		return ;
	m_cppimpl = true;

}

bool xsdSimpleContent::CheckCycle(xsdElement* elem)
{
	return false;
}

void xsdSimpleContent::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;

}

void xsdGroup::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;

}

void xsdExtension::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;

}

void xsdExtension::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{

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
	out.iprintln(indent,"void set(%s v) { m_value = v;} ",enumname) ;
	out.iprintln(indent,"const char * gets() const;") ;
	out.iprintln(indent,"%s get() const { return m_value;}",enumname) ;
	out.iprintln(indent,"%s m_value;",enumname);
}

void xsdEnum::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	/*
	 * generate code to convert this enum to a string
	 */
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
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

void xsdRestriction::GenHeader(CppFile & out,int indent,const char * defaultstr)
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
				out.iprintln(indent,"%s()",tname);
				out.iprintln(indent++,"{");
				if (defaultstr != NULL && *defaultstr)
				{
					out.iprintln(indent,"sets(\"%s\");",defaultstr);
				}
				out.iprintln(--indent,"}");
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
			}
			out.iprintln(indent,"%s m_value;",basename);
			out.iprintln(--indent,"};\n");
		}
		else if (m_simple != NULL)
		{
			m_simple->GenHeader(out,indent,defaultstr);
		}
	}
}

void xsdRestriction::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	printf("%s %s\n",__FUNCTION__,getCppName());
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
	if (m_enum != NULL)
	{
		m_enum->GenImpl(out,st,defaultstr);
	}
	else if (m_simple != NULL)
	{
		m_simple->GenImpl(out,st,defaultstr);
	}
	else
	{
	}
}

void xsdRestriction::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{

}

void xsdRestriction::setCppName(const char * name)
{
	if (m_enum != NULL)
		m_enum->setCppName(name);
}

bool xsdRestriction::isString()
{
	return m_base->isString() && getDim() != 1 ;
}

bool xsdRestriction::isScalar()
{
	bool res = false ;
	int d = getDim();
	if (d == 1)
		res = m_base->isInteger() || m_base->isfloat() || m_base->isString();
	return res ;
}

int xsdRestriction::getDim()
{
	int len = m_length;
	if (len == 0)
		len = m_maxLength;
	if (len == 0)
		len = 1 ;
	return len ;
}

int xsdList::getDim()
{
	return m_parent->getDim();
}

void xsdList::GenHeader(CppFile &out,int indent,const char * defaultstr)
{
	if (!m_hdrimpl)
	{
		m_hdrimpl = true;
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

			out.iprintln(indent,"xs_list<%s> m_list;",m_itemtype->getCppName());
			out.iprintln(--indent,"};\n");
		}
	}
}

void xsdList::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
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
	out.iprintln(indent,"%ssets(%s);",dest.getlvalue(),src);
}

void xsdList::setCppName(const char * name)
{
	m_parent->setCppName(name);
}

void xsdType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
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

void xsdComplexType::GenLocal(CppFile & out,Symtab & st,const char * defaultstr)
{
	m_type->GenLocal(out,st,defaultstr);
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

void xsdComplexType::GenImpl(CppFile & out,Symtab & st,const char * defaultstr)
{
	if (m_cppimpl)
		return ;
	m_cppimpl = true;
	m_type->GenLocal(out,st,defaultstr);
	out.iprintln(0,"/*\n * complexType %s\n*/",getName());
	out.iprintln(0,"void %s::Parse(xmlNodePtr node)",getQualifiedName().c_str());
	out.iprintln(0,"{");
	GenParserAttrLoop(out,st,m_attributes,defaultstr);
	if (m_type != NULL)
	{
		m_type->GenImpl(out,st,defaultstr);
	}
	out.iprintln(0,"}");
	out.println();
}

void xsdComplexType::GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src)
{
	if (dest.isPtr())
		out.iprintln(indent,"%s = new %s;",dest.getCppName(),dest.m_type->getCppName());
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



