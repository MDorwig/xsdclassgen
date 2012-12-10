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
#include <libxml2/libxml/parser.h>
#include "xsdclassgen.h"

void xsdSequence::GenHeader(CppFile & out,int indent)
{
	std::list<xsdElement*>::iterator ei;
	out.iprintln(indent,"struct %s",getCppName());
	out.iprintln(indent++,"{");
	out.iprintln(indent,"void Parse(xmlNodePtr node);");
	m_elements.GenHeader(out,indent);
	out.iprintln(--indent,"};");
}

void xsdAll::GenHeader(CppFile & out,int indent)
{
	std::list<xsdElement*>::iterator ei;
	out.iprintln(indent,"struct %s",getCppName());
	out.iprintln(indent++,"{");
	out.iprintln(indent,"void Parse(xmlNodePtr node);");
	for (ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenHeader(out,indent);
	}
	out.iprintln(--indent,"};\n");
}

void xsdExtension::GenHeader(CppFile &out,int indent)
{
	out.Indent(indent);
	out.printf("struct %s\n",getCppName());
	out.Indent(indent);
	out.printf("{\n");
	for (xsdAttrList::iterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
	{
		xsdAttribute * attr = *ai ;
		attr->GenHeader(out,indent+1);
	}
	out.Indent(indent+1);
	out.printf("%s %s;\n",m_basetypename->getCppName(),"m_val");
	out.Indent(indent);
	out.printf("};\n");
}

void xsdSimpleContent::GenHeader(CppFile &out,int indent)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_content != NULL)
		{
			m_content->GenHeader(out,indent);
		}
	}
}


void xsdChoice::GenHeader(CppFile &out,int indent)
{
	if (!m_impl)
	{
		m_impl = true ;
		out.iprintln(indent,"struct %s",getCppName());
		out.iprintln(indent++,"{");
		out.iprintln(indent,"void Parse(xmlNodePtr node);");
		m_elements.GenHeader(out,indent);
		out.iprintln(--indent,"};");
	}
}

void xsdComplexType::GenHeader(CppFile & out,int indent)
{
	if (!m_impl)
	{
		m_impl = true;
		if (!m_attributes.empty())
		{
#if 0
			out.Indent(indent);
			out.printf("struct %s\n",getCppName());
			out.Indent(indent);
			out.printf("{\n");
			for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
			{
				xsdAttribute * attr = *ai;
				attr->GenHeader(out,indent+1);
			}
			if (m_type != NULL)
				m_type->GenHeader(out,indent+1);
			out.Indent(indent);
			out.printf("} ;\n");
#endif
		}
		if (m_type != NULL)
		{
			m_type->GenHeader(out,indent);
		}
	}
}

void xsdElement::GenHeader(CppFile &out,int indent)
{
	printf("GenHeader for elem %s\n",getCppName());
	for (typeIterator ti = m_deplist.begin() ; ti != m_deplist.end() ; ti++)
	{
		xsdType * type = *ti;
		if (type->isLocal())
		{
			type->GenHeader(out,indent);
		}
	}
	if (m_type == NULL)
	{
		printf("Element %s has unknown type %s\n",getName(),m_typename->m_name.c_str());
		out.iprintln(indent,"%s %s;\n",m_typename->m_name.c_str(),getCppName());
	}
	else
	{
		int len = m_maxOccurs;
		if (len > 1)
			out.iprintln(indent,"%s %s[%d];",m_type->getCppName(),getCppName(),len);
		else
			out.iprintln(indent,"%s %s;",m_type->getCppName(),getCppName());
	}
}


void xsdElement::GenImpl(CppFile & out,Symtab & st)
{
	for (typeIterator ti = m_deplist.begin() ; ti != m_deplist.end() ; ti++)
	{
		xsdType * t = *ti ;
		t->GenImpl(out,st);
	}
}

void xsdElement::GenLocal(CppFile & out,Symtab & st)
{
	if (m_type->isLocal())
	{
		m_type->GenImpl(out,st);
	}
}

void xsdGroup::GenHeader(CppFile & out,int indent)
{
	if (m_type != NULL)
		m_type->GenHeader(out,indent);
}

void xsdGroup::GenLocal(CppFile & out,Symtab & st)
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

void xsdAttribute::GenHeader(CppFile & out,int indent)
{
	if (m_type != NULL)
	{
		out.Indent(indent);
		if (m_fixed.empty())
			out.printf("%s %s;\n",m_type->getCppName(),m_cname.c_str());
		else
		{
			out.printf("char %s[%d+1];\n",getCppName(),m_fixed.size());
		}
	}
}

const char * xsdRestriction::getReturnType()
{
	static std::string s ;
	s = m_base->getCppName();
	if (!isScalar())
		s += " * ";
	return s.c_str() ;
}

void xsdTypeList::GenHeader(CppFile & out,int indent,bool choice)
{
	for (typeIterator ti = begin() ; ti != end() ; ti++)
	{
		(*ti)->GenHeader(out,indent);
	}
}

void xsdSimpleType::GenHeader(CppFile & out,int indent)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_rest != NULL)
			m_rest->GenHeader(out,indent);
		else if (m_list != NULL)
			m_list->GenHeader(out,indent);
		else if (m_union != NULL)
			m_union->GenHeader(out,indent);
	}
}

void xsdType::GenHeader(CppFile & out,int indent)
{

}

void xsdAll::GenImpl(CppFile & out,Symtab & st)
{

}

void xsdAll::GenLocal(CppFile & out,Symtab & st)
{

}

void xsdSequence::GenImpl(CppFile & out,Symtab & st)
{
	out.iprintln(0,"/*\n * sequence\n*/");
	for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
	{
		xsdElement * e = *ei ;
		if (e->m_maxOccurs > 1)
			out.iprintln(1,"int %s = 0;",e->getIndexVar());
	}
	out.iprintln(1,"for_each_child(child,node)");
	out.iprintln(1,"{");
	out.iprintln(2,"if (child->type == XML_ELEMENT_NODE)");
	out.iprintln(2,"{");
	out.iprintln(3,"symbols kw = Lookup(child->name);");
	out.iprintln(3,"switch(kw)");
	out.iprintln(3,"{");
	for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
	{
		xsdElement * e = *ei ;
		Symbol * s = st.find(e->m_name.c_str());
		if (s != NULL)
		{
			out.iprintln(4,"case sy_%s:",s->m_cname.c_str());
			if (e->m_type != NULL)
			{
				std::string m = e->getCppName();
				if (e->m_maxOccurs > 1)
				{
					out.iprintln(5,"if (%s < %d)",e->getIndexVar(),e->m_maxOccurs);
					m += "[";
					m += e->getIndexVar();
					m += "++]";
					e->m_type->GenAssignment(out,6,m.c_str(),"getContent(child)");
				}
				else
				{
					e->m_type->GenAssignment(out,5,m.c_str(),"getContent(child)");
				}
			}
			out.iprintln(4,"break;");
		}
	}
	out.iprintln(4,"default:");
	out.iprintln(4,"break;");
	out.iprintln(3,"}");
	out.iprintln(2,"}");
	out.iprintln(1,"}");
}

void xsdSequence::GenLocal(CppFile & out,Symtab & st)
{
	m_elements.GenLocal(out,st);
}

void xsdChoice::GenImpl(CppFile & out,Symtab & st)
{
	out.iprintln(0,"/*\n * choice\n*/");
	out.iprintln(0,"for_each_child(child,node)");
	out.iprintln(0,"{");
	out.iprintln(0,"}");
}

void xsdChoice::GenLocal(CppFile & out,Symtab & st)
{

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

void xsdSimpleType::GenImpl(CppFile & out,Symtab & st)
{
	out.iprintln(0,"/*\n * simpleType %s\n*/",getName());
	if (m_rest != NULL)
		m_rest->GenImpl(out,st);
	else if (m_list != NULL)
		m_list->GenImpl(out,st);
	else if (m_union != NULL)
		m_union->GenImpl(out,st);
}

void xsdSimpleType::GenLocal(CppFile & out,Symtab & st)
{

}

void xsdSimpleType::GenAssignment(CppFile & out,int indent,const char * leftside,const char * rightside)
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

void xsdSimpleContent::GenImpl(CppFile & out,Symtab & st)
{

}

void xsdSimpleContent::GenLocal(CppFile & out,Symtab & st)
{

}

void xsdGroup::GenImpl(CppFile & out,Symtab & st)
{

}

void xsdExtension::GenImpl(CppFile & out,Symtab & st)
{

}

void xsdExtension::GenLocal(CppFile & out,Symtab & st)
{

}

void xsdEnum::GenHeader(CppFile & out,int indent)
{
	/*
	 * enums werden in einer struktur gekapselt um die get/set Methoden zu implementieren
	 */
	const char * enumname = getCppName();
	out.iprintln(indent,"enum %s",enumname);
	out.iprintln(indent++,"{");
	enumIterator si ;
	for (si = m_values.begin() ; si != m_values.end() ; si++)
	{
		xsdEnumValue * v = *si ;
		out.iprintln(indent,"%s,",v->getCppName());
	}
	out.iprintln(--indent,"} ;");
	out.iprintln(indent,"void sets(const char *); // set from string");
	out.iprintln(indent,"void set(%s v) { m_value = v;} ",enumname) ;
	out.iprintln(indent,"const char * gets(); // conver to string") ;
	out.iprintln(indent,"%s get() { return m_value;}",enumname) ;
	out.iprintln(indent,"%s m_value;",enumname);
}

void xsdEnum::GenImpl(CppFile & out,Symtab & st)
{
	/*
	 * generate code to convert this enum to a string
	 */
	std::string qname = m_parent->getQualifiedName();
	out.iprintln(0,"const char * %s::gets()",qname.c_str());
	out.iprintln(0,"{");
	  out.iprintln(1,"switch(m_value)");
	  out.iprintln(1,"{");
	  for (enumIterator ei = m_values.begin() ; ei != m_values.end() ; ei++)
	  {
		  xsdEnumValue * v = *ei ;
			out.iprintln(2,"case %s: return \"%s\";",v->getCppName(),v->m_Value.c_str());
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
			out.iprintln(2,"case sy_%s: m_value = %s; break;",val->m_Value.c_str(),val->getCppName());
	  }
	    out.iprintln(2,"default: m_value = (%s)-1; break;",getCppName());
	  out.iprintln(1,"}");
	out.iprintln(0,"}\n");
}


void xsdRestriction::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest,src);
}

void xsdRestriction::GenHeader(CppFile & out,int indent)
{
	const char * tname = getCppName();
	if (m_enum != NULL)
	{
		out.iprintln(indent,"struct %s",tname);
		out.iprintln(indent++,"{");
		m_enum->GenHeader(out,indent);
		out.iprintln(--indent,"};");
	}
	else
	{
		if (m_base != NULL)
		{
			int d = getDim();
			const char * basename = m_base->getCppName();
			out.iprintln(indent,"struct %s",tname);
			out.iprintln(indent++,"{");
			out.iprintln(indent,"void sets(const char * str);");
			if (m_base->isScalar())
			{
				out.iprintln(indent,"void set(const %s arg);",getReturnType());
				out.iprintln(indent,"%s get();",getReturnType());
			}
			else
			{
				out.iprintln(indent,"void set(const %s * arg);",getReturnType());
				out.iprintln(indent,"%s * get();",getReturnType());
			}
			out.iprintln(indent,"const char * gets();");
			if (d > 1)
			{
				out.iprintln(indent,"%s m_value[%d];",basename,d);
			}
			else
			{
				out.iprintln(indent,"%s m_value;",basename);
			}
			out.iprintln(--indent,"};");
		}
		else if (m_simple != NULL)
		{
			m_simple->GenHeader(out,indent);
		}
	}
}

void xsdRestriction::GenImpl(CppFile & out,Symtab & st)
{
	if (m_enum != NULL)
	{
		m_enum->GenImpl(out,st);
	}
	else if (m_simple != NULL)
	{
		m_simple->GenImpl(out,st);
	}
	else
	{
		const char * basename = m_base->getCppName();
		out.iprintln(0,"void %s::sets(const char * str)",getQualifiedName().c_str());
		out.iprintln(0,"{");
		if (isInteger())
		{
			out.iprintln(1,"%s t;",basename);
			if (m_base->isSignedInteger())
				out.iprintln(1,"t = strtol(str,NULL,10);");
			else
				out.iprintln(1,"t = strtoul(str,NULL,10)");
			out.iprintln(1,"set(t);");
		}
		else if (isString())
		{
			out.iprintln(1,"strncpy(m_value,str,sizeof m_value-1);");
			out.iprintln(1,"m_value[sizeof m_value-1] = 0;");
		}
		else if (isChar())
		{
			out.iprintln(1,"m_value = str[0];");
		}
		out.iprintln(0,"}");
		out.iprintln(0,"void %s::set(const %s val)",getQualifiedName().c_str(),getReturnType());
		out.iprintln(0,"{");
		if (isString())
		{
			out.iprintln(1,"strncpy(m_value,val,sizeof m_value-1);");
			out.iprintln(1,"m_value[sizeof m_value-1] = 0;");
		}
		else if (isScalar())
			out.iprintln(1,"m_value = val;");
		else
			out.iprintln(1,"memcpy(m_value,val,sizeof m_value);");
		out.iprintln(0,"}");
		out.iprintln(0,"%s %s::get()",getReturnType(),getQualifiedName().c_str());
		out.iprintln(0,"{");
		out.iprintln(0,"  return m_value;");
		out.iprintln(0,"}");
		/*
		 * generate the gets function
		 * it conerts m_value into a string
		 */
		out.iprintln(0,"const char * %s::gets()",getQualifiedName().c_str());
		out.iprintln(0,"{");
		if (isString())
			out.iprintln(1,"return m_value;");
		else if (isChar())
		{
			out.iprintln(1,"static char res[2];");
			out.iprintln(1,"res[0] = m_value;");
			out.iprintln(1,"res[1] = 0;");
			out.iprintln(1,"return res;");
		}
		else if (isInteger())
		{
			out.iprintln(1,"static char res[10];");
			out.iprintln(1,"sprintf(res,\"%%d\",m_value);");
			out.iprintln(1,"return res;");
		}
		else if (isfloat())
		{
			out.iprintln(1,"static char res[10];");
			out.iprintln(1,"sprintf(res,\"%%g\",m_value;");
			out.iprintln(1,"return res;");
		}
		out.iprintln(0,"}");
	}
}

void xsdRestriction::GenLocal(CppFile & out,Symtab & st)
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

bool xsdRestriction::isChar()
{
	return m_base->isString() && getDim() == 1 ;
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

void xsdList::GenHeader(CppFile &out,int indent)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_itemtype != NULL)
		{
			int d = getDim();
			out.iprintln(indent,"struct %s",getCppName());
			out.iprintln(indent++,"{");
			out.iprintln(indent,    "void sets(const char * str);");
			out.iprintln(indent,    "const char * gets(const char * str);");
			if (d > 1)
			{
				out.iprintln(indent,"int m_count;");
				out.iprintln(indent,"%s m_value[%d];",m_itemtype->getCppName(),d);
			}
			else
				out.iprintln(indent,"%s m_value;",m_itemtype->getCppName());
			out.iprintln(--indent,"};");
		}
	}
}

void xsdList::GenImpl(CppFile & out,Symtab & st)
{
	if (m_itemtype == NULL)
		m_itemtype = FindType(m_itemtypename);
	if (m_itemtype->isLocal())
		m_itemtype->GenImpl(out,st);
	out.iprintln(0,"void %s::sets(const char * str)",getQualifiedName().c_str());
	out.iprintln(0,"{");
	if (getDim() == 1)
		m_itemtype->GenAssignment(out,1,"m_value","str");
	else
	{
		/*
		 *
		 */
		int ilen = getDim();
		out.iprintln(1,"char * s = strdup(str);");
		out.iprintln(1,"char * p = s;");
		out.iprintln(1,"m_count = 0;");
		out.iprintln(1,"char * t = mystrtok(&p,\" \");");
		out.iprintln(1,"for(int i = 0 ; t != NULL &&  i < %d ; i++)",ilen);
		out.iprintln(1,"{");
		m_itemtype->GenAssignment(out,2,"m_value[i]","t");
		out.iprintln(2,"m_count++;");
		out.iprintln(2,"t = mystrtok(&p,\" \");");
		out.iprintln(1,"}");
		out.iprintln(1,"free(s);");
	}
	out.iprintln(0,"}");
}

void xsdList::GenLocal(CppFile & out,Symtab & st)
{

}

void xsdList::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
	out.iprintln(indent,"%s.sets(%s);",dest,src);
}

void xsdList::setCppName(const char * name)
{
	m_parent->setCppName(name);
}

void xsdType::GenImpl(CppFile & out,Symtab & st)
{

}

void xsdType::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
	if (isInteger())
	{
		if (isUnsignedInteger())
		  out.iprintln(indent,"%s = strtoul(%s,NULL,10);",dest,src);
		else
			out.iprintln(indent,"%s = strtol(%s,NULL,10);",dest,src);
	}
	else if (isString())
	{
		out.iprintln(indent,"strcpy(%s,%s);",dest,src);
	}
	else if (isfloat())
	{
		if (m_tag == type_double)
			out.iprintln(indent,"%s = strtod(%s,NULL);",dest,src);
		else
			out.iprintln(indent,"%s = strtof(%s,NULL);",dest,src);
	}
	else
	{
		out.iprintln(indent,"// Assignement %s = %s not implemented",dest,src);
	}
}

void xsdComplexType::GenLocal(CppFile & out,Symtab & st)
{
	m_type->GenLocal(out,st);
}

void xsdComplexType::GenImpl(CppFile & out,Symtab & st)
{
	out.iprintln(0,"/*\n * complexType %s\n*/",getName());
	out.iprintln(0,"void %s::Parse(xmlNodePtr node)",getQualifiedName().c_str());
	out.iprintln(0,"{");
	if (!m_attributes.empty())
	{
		out.iprintln(1,"for_each_attr(attr,node)");
		out.iprintln(1,"{");
		out.iprintln(2,"symbols kw;");
		out.iprintln(2,"kw=Lookup(attr->name);");
		out.iprintln(2,"switch(kw)");
		out.iprintln(2,"{");
		for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
		{
			xsdAttribute * a = *ai ;
			Symbol * s = st.find(a->m_name.c_str());
			if (s != NULL)
			{
				out.iprintln(3,"case sy_%s:",s->m_cname.c_str());
				a->m_type->GenAssignment(out,4,a->getCppName(),"getContent(attr)");
				out.iprintln(3,"break;");
			}
		}
		out.iprintln(3,"      default:");
		out.iprintln(3,"      break;");
		out.iprintln(2,"    }");
		out.iprintln(1,"  }");
  }
	if (m_type != NULL)
	{
		m_type->GenImpl(out,st);
	}
	out.iprintln(0,"}");
	out.println();
}

void xsdComplexType::GenAssignment(CppFile & out,int indent,const char * dest,const char * src)
{
	out.iprintln(indent,"%s.Parse(child);",dest);
}
