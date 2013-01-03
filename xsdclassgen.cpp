/*
 * xsdclassgen.cpp
 *
 *  Created on: 26.11.2012
 *      Author: dorwig
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <libxml2/libxml/parser.h>
#include "xsdclassgen.h"
#include "cppfile.h"

enum xsd_keyword
{
  xsd_all,
  xsd_annotation,
  xsd_any,
  xsd_attribute,
  xsd_base,
  xsd_choice,
  xsd_complexContent,
  xsd_complexType,
  xsd_default,
  xsd_element,
  xsd_enumeration,
  xsd_extension,
  xsd_fixed,
  xsd_fractionDigits,
  xsd_group,
  xsd_id,
  xsd_import,
  xsd_int,
  xsd_itemType,
  xsd_length,
  xsd_list,
  xsd_maxExclusive,
  xsd_maxInclusive,
  xsd_maxLength,
  xsd_maxOccurs,
  xsd_minExclusive,
  xsd_minInclusive,
  xsd_minLength,
  xsd_minOccurs,
  xsd_name,
  xsd_none,
  xsd_optional,
  xsd_pattern,
  xsd_prohibited,
  xsd_required,
  xsd_restriction,
  xsd_schema,
  xsd_schemaLocation,
  xsd_sequence,
  xsd_simpleContent,
  xsd_simpleType,
  xsd_string,
  xsd_targetNamespace,
  xsd_totalDigits,
  xsd_type,
  xsd_union,
  xsd_use,
  xsd_value,
  xsd_whiteSpace,
  xsd_xmlns,
};

struct xsd_keyword_entry
{
	xsd_keyword keyword;
	const char * name ;
};

#define ENTRY(x) {xsd_##x,#x}

struct xsd_keyword_entry keywordtable[] = {
		ENTRY(all),
		ENTRY(annotation),
		ENTRY(any),
		ENTRY(attribute),
		ENTRY(base),
    ENTRY(choice),
    ENTRY(complexContent),
    ENTRY(complexType),
    ENTRY(default),
    ENTRY(element),
    ENTRY(enumeration),
    ENTRY(extension),
    ENTRY(fixed),
    ENTRY(fractionDigits),
    ENTRY(group),
    ENTRY(id),
    ENTRY(import),
    ENTRY(int),
    ENTRY(itemType),
    ENTRY(length),
    ENTRY(list),
    ENTRY(maxExclusive),
    ENTRY(maxInclusive),
    ENTRY(maxLength),
    ENTRY(maxOccurs),
    ENTRY(minExclusive),
    ENTRY(minInclusive),
    ENTRY(minLength),
    ENTRY(minOccurs),
    ENTRY(name),
    ENTRY(optional),
    ENTRY(pattern),
    ENTRY(prohibited),
    ENTRY(required),
    ENTRY(restriction),
    ENTRY(schema),
  	ENTRY(schemaLocation),
    ENTRY(sequence),
    ENTRY(simpleContent),
    ENTRY(simpleType),
    ENTRY(string),
  	ENTRY(targetNamespace),
    ENTRY(totalDigits),
    ENTRY(type),
    ENTRY(union),
    ENTRY(use),
    ENTRY(value),
    ENTRY(whiteSpace),
  	ENTRY(xmlns),
};

int xsdkeywordcmp(const void * p1,const void * p2)
{
	const char * key = (const char *)p1 ;
	const xsd_keyword_entry * e = (xsd_keyword_entry*)p2;
	return strcmp(key,e->name);
}

xsd_keyword Lookup(const xmlChar * name)
{
	xsd_keyword kw = xsd_none;
	xsd_keyword_entry * entry = (xsd_keyword_entry*)bsearch(name,keywordtable,sizeof keywordtable/sizeof keywordtable[0],sizeof keywordtable[0],xsdkeywordcmp);
	if (entry != NULL)
		kw = entry->keyword;
	return kw ;
}

std::list<std::string*> includePaths ;

void AddIncludePath(const char * path)
{
	std::string * s = new std::string(path);
	if (s->at(s->size()-1) == '/')
		s->erase(s->size()-1);
	includePaths.push_back(s);
}

int xsdType::m_count;

class xsdNamespace
{
public:
	xsdNamespace(const char * name,const char * href)
	{
		m_name = name ;
		m_href = href ;
	}

	xsdType * FindType(const char * name)
	{
		if (*name == 0)
			return NULL;
		for (typeIterator ti = m_types.begin() ; ti != m_types.end() ; ti++)
		{
			xsdType * type = *ti ;
			if (!type->isLocal())
			{
				if (type->m_name == name)
					return type ;
			}
		}
		return NULL;
	}

	void AddType(xsdType * type)
	{
		if (FindType(type->getName()) == NULL)
		{
			m_types.push_back(type);
		}
	}

	xsdElement * FindElement(const char * name)
	{
		for (elementIterator ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = *ei ;
			if (elem->m_name == name)
				return elem ;
		}
		return NULL;
	}

	void AddElement(xsdElement * elem)
	{
		if (FindElement(elem->getCppName()) == NULL)
		{
			m_elements.push_back(elem);
		}
	}

	std::string    m_name ;
	std::string    m_href;
	xsdTypeList    m_types;
	xsdElementList m_elements;
	xsdTypeList 	 m_deplist;

	void GenSymtab(Symtab & st)
	{
	}
} ;

typedef std::list<xsdNamespace*> NamespaceList ;
NamespaceList namespaces;
xsdNamespace *targetNamespace;

std::string MakeIdentifier(const char * prefix,const char * name)
{
	std::string str ;
	if (prefix != NULL)
		str += prefix ;
	while(*name)
	{
		unsigned char c = *name++;
		if (!isalnum(c) && c != '_')
		{
			char n[8];
			if (*name)
				sprintf(n,"_x%02x_",c);
			else
				sprintf(n,"_x%02x",c);
			str += n ;
		}
		else
			str += c ;
	}
	return str ;
}

xsdNamespace * FindNamespaceRef(const char * href)
{
	for (NamespaceList::iterator nsi = namespaces.begin() ; nsi != namespaces.end() ; nsi++)
	{
		xsdNamespace * ns = * nsi ;
		if (ns->m_href == href)
			return ns ;
	}
	return NULL;
}

xsdNamespace * FindNamespace(const char * prefix)
{
	for (NamespaceList::iterator nsi = namespaces.begin() ; nsi != namespaces.end() ; nsi++)
	{
		xsdNamespace * ns = * nsi ;
		if (ns->m_name == prefix)
			return ns ;
	}
	return NULL;
}

xsdNamespace * AddNamespace(const char * prefix,const char * href)
{
	xsdNamespace * ns = FindNamespace(prefix);
	if (ns == NULL)
	{
		ns = new xsdNamespace(prefix,href);
		namespaces.push_front(ns);
	}
	return ns ;
}

xsdType * FindType(const char * name)
{
	xsdType * type = NULL;
	std::string prefix;
	xsdNamespace * ns = targetNamespace;
	if (strchr(name,':') != NULL)
	{
		while(*name != ':')
			prefix += *name++;
		name++;
		ns = FindNamespace(prefix.c_str());
	}
	if (ns != NULL)
	{
		type = ns->FindType(name);
	}
	return type ;
}

xsdType * FindType(xsdTypename * tn)
{
	xsdType * type ;
	xsdNamespace * ns ;
	if (tn->m_ns.empty())
		ns = targetNamespace;
	else
		ns = FindNamespace(tn->m_ns.c_str());
	if (ns != NULL)
	{
		type = ns->FindType(tn->m_name.c_str());
	}
	return type ;
}

void AddType(xsdType * type)
{
	if (targetNamespace == NULL)
	{
			printf("no targetNamespace\n");
			targetNamespace = AddNamespace("","");
	}
	xsdNamespace * ns = targetNamespace;
	if (!type->m_ns.empty())
	{
		ns = FindNamespace(type->m_ns.c_str());
	}
	if (ns != NULL)
	{
		ns->AddType(type);
	}
}

#define for_each_child(child,node) for (xmlNodePtr child = node->children   ; child != NULL ; child = child->next)
#define for_each_attr(attr,node)   for (xmlAttrPtr attr  = node->properties ; attr != NULL  ; attr = attr->next)
#define for_each_nsdef(nsdef,node) for (xmlNsPtr nsdef = node->nsDef ; nsdef != NULL ; nsdef = nsdef->next)

const char * getContent(xmlNodePtr node)
{
	const char * c = "" ;
	if (node != NULL)
		c = (const char*)node->content;
	return c ;
}

xsdElement * FindElement(const char * name)
{
	xsdElement * elem = NULL;
	std::string prefix ;
	xsdNamespace * ns = targetNamespace;
	if (strchr(name,':') != NULL)
	{
		while(*name != ':')
			prefix += *name++;
		name++;
		ns = FindNamespace(prefix.c_str());
	}
	if (ns != NULL)
	{
		elem = ns->FindElement(name);
	}
	return elem ;
}

void AddElement(xsdElement * elem)
{
	if (targetNamespace == NULL)
	{
			printf("no targetNamespace\n");
			targetNamespace = AddNamespace("","");
	}
	xsdNamespace * ns = targetNamespace;
	if (!elem->m_ns.empty())
		ns = FindNamespace(elem->m_ns.c_str());
	if (ns != NULL)
		ns->AddElement(elem);
}

void not_allowed_error(xmlNodePtr e,xmlNodePtr n)
{
	printf("%s:%d \"%s\" not allowed in \"%s\"\n",e->doc->URL,e->line,e->name,n->name);
}

void not_allowed_error(xmlAttrPtr e,xmlNodePtr n)
{
	printf("%s:%d \"%s\" not allowed in \"%s\"\n",e->doc->URL,e->parent->line,e->name,n->name);
}

void not_supported_error(xmlNodePtr e,xmlNodePtr n)
{
	printf("%s:%d \"%s\" not supported in \"%s\"\n",e->doc->URL,e->line,e->name,n->name);
}

void not_supported_error(xmlAttrPtr a,xmlNodePtr n)
{
	printf("%s:%d \"%s\" not supported in \"%s\"\n",a->doc->URL,a->parent->line,a->name,n->name);
}

void not_supported_error(xmlNodePtr e,xmlDocPtr doc)
{
	printf("%s:%d \"%s\" not supported in \"%s\"\n",doc->URL,e->line,e->name,doc->name);
}

xsdRestriction * ParseRestriction(xmlNodePtr rest,xsdType * parent,Symtab & st);
xsdComplexType * ParseComplexType(xmlNodePtr type,xsdElement * elem,xsdType * parent,Symtab & st);
xsdSimpleType  * ParseSimpleType(xmlNodePtr type,const char * elemname,xsdType * parent,Symtab & st);
xsdChoice      * ParseChoice(xmlNodePtr choice,xsdType * ,Symtab & st);
xsdGroup  		 * ParseGroup(xmlNodePtr group,xsdType * parent,Symtab & st);

xsdAttribute   * ParseAttribute(xmlNodePtr node,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	const char * xsdname = "";
	const char * xsddefault = "";
	const char * xsdfixed = "" ;
	xsdType    * xsdtype = NULL;
	xsdAttribute::eUse use = xsdAttribute::eUse_Optional;
	for_each_attr(attr,node)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdname = getContent(attr->children);
				st.add(xsdname);
			break ;

			case	xsd_default:
				xsddefault = getContent(attr->children);
			break ;

			case	xsd_type:
			{
				xsdtype = FindType(getContent(attr->children));
			}
			break ;

			case	xsd_fixed:
				xsdfixed = getContent(attr->children);
			break ;

			case	xsd_use:
				kw = Lookup(attr->children->content);
				switch(kw)
				{
					case	xsd_optional  : use = xsdAttribute::eUse_Optional; break ;
					case	xsd_prohibited: use = xsdAttribute::eUse_Prohibited; break ;
					case	xsd_required  : use = xsdAttribute::eUse_Required; break ;
					default:
					break ;
				}
			break ;

			default:
				not_supported_error(attr,node);
			break ;
		}
		for_each_child(child,node)
		{
			if (child->type == XML_ELEMENT_NODE)
			{
				kw = Lookup(child->name);
				switch(kw)
				{
					case	xsd_simpleType:
						if (xsdtype == NULL)
							xsdtype = ParseSimpleType(child,xsdname,parent,st);
					break ;

					case	xsd_annotation:
					break ;

					default:
						not_supported_error(child,node);
					break ;
				}
			}
		}
	}
	st.m_hasAttributes = true ;
	xsdAttribute * xsdattr = new xsdAttribute(xsdname,xsddefault,xsdfixed,use,xsdtype);
	return xsdattr;
}

xsdElement * ParseElement(xmlNodePtr element,xsdType * parent,Symtab & st)
{
	xsdElement * xsdelem     = NULL;
	xsdType    * xsdtype     = NULL;
	const char * xsdname     = NULL;
	const char * xsddefault  = NULL;
	xsdTypename* xsdtypename = NULL;
	int minOccurs = 1 ;
	int maxOccurs = 1 ;
	bool isChoice = parent != NULL && parent->m_tag == type_choice;
	for_each_attr(attr,element)
	{
		xsd_keyword kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdname = getContent(attr->children);
				st.add(xsdname);
			break ;

			case	xsd_type:
				xsdtypename = new xsdTypename(getContent(attr->children));
			break ;

			case	xsd_minOccurs:
				minOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			case	xsd_maxOccurs:
				if (strcmp(getContent(attr->children),"unbounded") == 0)
					maxOccurs = 0 ;
				else
					maxOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			case	xsd_default:
				xsddefault = getContent(attr->children);
			break ;

			default:
				not_supported_error(attr,element);
			break ;
		}
	}
	xsdelem = new xsdElement(xsdname,xsdtypename,xsdtype,minOccurs,maxOccurs,isChoice,xsddefault);
	for_each_child(child,element)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			xsd_keyword kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_complexType:
					if (xsdelem->m_type != NULL)
						printf("element %s already has a type %s\n",xsdname,xsdtype->getName());
					else
						ParseComplexType(child,xsdelem,parent,st);
				break ;

				case	xsd_simpleType:
					if (xsdelem->m_type != NULL)
						printf("element %s already has a type %s\n",xsdname,xsdtype->getCppName());
					else
						xsdelem->m_type = ParseSimpleType(child,xsdname,parent,st);
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,element);
				break ;
			}
		}
	}
	return xsdelem;
}

xsdSequence * ParseSequence(xmlNodePtr sequence,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	xsdSequence * xsdseq = new xsdSequence(parent);
	for_each_attr(attr,sequence)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_minOccurs:
				xsdseq->m_minOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			case	xsd_maxOccurs:
				xsdseq->m_maxOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			default:
				not_supported_error(attr,sequence);
			break ;
		}
	}
	for_each_child(child,sequence)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_element:
					xsdseq->m_elements.push_back(ParseElement(child,xsdseq,st));
				break ;
				case	xsd_group:
					xsdseq->m_types.push_back(ParseGroup(child,xsdseq,st));
				break ;

				case	xsd_choice:
					xsdseq->m_types.push_back(ParseChoice(child,xsdseq,st));
				break ;
				case  xsd_sequence:
				{
					xsdSequence * s = ParseSequence(child,xsdseq,st);
					while (!s->m_elements.empty())
					{
						xsdElement * e = *s->m_elements.begin() ;
						xsdseq->m_elements.push_back(e);
						s->m_elements.remove(e);
					}
					delete s ;
				}
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,sequence);
				break ;
			}
		}
	}
	return xsdseq;
}

xsdAll * ParseAll(xmlNodePtr all,xsdType * parent,Symtab & st)
{
	xsdAll * xsdtype ;
	xsd_keyword kw ;
	for_each_attr(attr,all)
	{

	}
	xsdtype = new xsdAll(parent);
	for_each_child(child,all)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_element:
					xsdtype->m_elements.push_back(ParseElement(child,xsdtype,st));
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,all);
				break ;
			}
		}
	}
	return xsdtype;
}

xsdGroup  * ParseGroup(xmlNodePtr group,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	const char * xsdname = NULL;
	xsdGroup * xsdgroup = NULL;
	for_each_attr(attr,group)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdname = getContent(attr->children);
			break ;

			default:
				not_supported_error(attr,group);
			break ;
		}
	}
	xsdgroup = new xsdGroup(xsdname,parent) ;

	for_each_child(child,group)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_all:
					xsdgroup->m_type = ParseAll(child,xsdgroup,st);
				break ;

				case	xsd_choice:
					xsdgroup->m_type = ParseChoice(child,xsdgroup,st);
				break ;

				case	xsd_sequence:
					xsdgroup->m_type = ParseSequence(child,xsdgroup,st);
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,group);
				break ;
			}
		}
	}
	return xsdgroup;
}

xsdChoice * ParseChoice(xmlNodePtr choice,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	int min = 0,max = 0 ;
	for_each_attr(attr,choice)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_maxOccurs:
				max = strtol(getContent(attr->children),NULL,10);
			break ;

			case	xsd_minOccurs:
				min = strtol(getContent(attr->children),NULL,10);
			break ;

			default:
				not_supported_error(attr,choice);
			break ;
		}
	}
	xsdChoice * xsdchoice = new xsdChoice(min,max,parent);
	for_each_child(child,choice)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_element:
				{
					xsdElement * elem = ParseElement(child,xsdchoice,st);
					xsdchoice->m_elements.push_back(elem);
				}
				break ;


				case	xsd_choice:
				{
					xsdChoice * c = ParseChoice(child,xsdchoice,st);
					/*
					 * alle Elemente der untergeorneten choice in diese choice übertragen
					 */
					while (!c->m_elements.empty())
					{
						xsdElement * elem = *c->m_elements.begin();
						xsdchoice->m_elements.push_back(elem);
						c->m_elements.remove(elem);
					}
				}
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,choice);
				break ;
			}
		}
	}

	return xsdchoice;
}

xsdExtension * ParseExtension(xmlNodePtr ext,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	xsdExtension * xsdext = NULL;
	xsdType  * base = NULL;
	for_each_attr(attr,ext)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_base:
				base = FindType(getContent(attr->children));
			break ;
			default:
			break ;
		}
	}
	xsdext = new xsdExtension(base,parent);
	for_each_child(child,ext)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_attribute:
					xsdext->m_attributes.push_back(ParseAttribute(child,xsdext,st));
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,ext);
				break ;
			}
		}
	}
	return xsdext;
}

xsdSimpleContent * ParseSimpleContent(xmlNodePtr cont,xsdType * parent,Symtab & st)
{
	xsdSimpleContent * xsdcontent = new xsdSimpleContent(parent);
	xsd_keyword kw ;
	for_each_child(child,cont)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xsdcontent->m_content = ParseRestriction(child,xsdcontent,st);
				break ;

				case	xsd_extension:
					xsdcontent->m_content = ParseExtension(child,xsdcontent,st);
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,cont);
				break ;
			}
		}
	}
	return xsdcontent;
}

xsdComplexType * ParseComplexType(xmlNodePtr type,xsdElement * elem,xsdType * parent,Symtab & st)
{
	const char * xsdtypename = "";
	xsd_keyword kw ;
	for_each_attr(attr,type)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdtypename = getContent(attr->children);
			break ;

			default:
			break ;
		}
	}
	xsdComplexType * newtype = new xsdComplexType(xsdtypename,elem,parent);
	for_each_child(child,type)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name) ;
			switch(kw)
			{
				case  xsd_attribute:
					newtype->m_attributes.push_back(ParseAttribute(child,newtype,st));
				break ;

				case	xsd_sequence:
					newtype->m_type = ParseSequence(child,newtype,st);
				break ;

				case	xsd_choice:
					newtype->m_type = ParseChoice(child,newtype,st);
				break ;

				case	xsd_all:
					newtype->m_type = ParseAll(child,newtype,st);
				break ;

				case	xsd_simpleContent:
					newtype->m_type = ParseSimpleContent(child,newtype,st);
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,type);
				break ;
			}
		}
	}
	return newtype;
}

xsdEnumValue * ParseEnum(xmlNodePtr ptr,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	for_each_attr(attr,ptr)
	{
		kw = Lookup(attr->name);
		if (kw == xsd_value)
		{
			const char * value;
			value = getContent(attr->children);
			st.add(value);
			return new xsdEnumValue(value);
		}
	}
	return NULL ;
}

int getIntAttr(xmlNodePtr ptr,const char * name)
{
	for_each_attr(attr,ptr)
	{
		if (strcmp((const char*)attr->name,name) == 0)
		{
			return strtol(getContent(attr->children),NULL,10);
		}
	}
	return 0 ;
}

xsdRestriction * ParseRestriction(xmlNodePtr rest,xsdType * parent,Symtab & st)
{
	xsdRestriction * xsdrest = new xsdRestriction(parent);
	xsd_keyword kw ;
	for_each_attr(attr,rest)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_base:
				xsdrest->m_base = FindType(getContent(attr->children));
			break ;
			default:
			break ;
		}
	}
	for_each_child(child,rest)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_simpleType:
					if (xsdrest->m_simple == NULL)
						xsdrest->m_simple= ParseSimpleType(child,"",xsdrest,st);
				break ;

				case	xsd_enumeration:
					if (xsdrest->m_enum == NULL)
						xsdrest->m_enum = new xsdEnum(xsdrest);
					xsdrest->m_enum->AddValue(ParseEnum(child,xsdrest,st));
				break ;

				case	xsd_minExclusive:
					xsdrest->m_minExclusive = getIntAttr(child,"value");
				break ;

				case	xsd_maxExclusive:
					xsdrest->m_maxExclusive = getIntAttr(child,"value");;
				break ;

				case	xsd_minInclusive:
					xsdrest->m_minInclusive = getIntAttr(child,"value");;
				break ;

				case	xsd_maxInclusive:
					xsdrest->m_maxInclusive = getIntAttr(child,"value");;
				break ;

				case	xsd_totalDigits:
					xsdrest->m_totalDigits = getIntAttr(child,"value");
				break ;

				case	xsd_fractionDigits:
					xsdrest->m_fractionDigits= getIntAttr(child,"value");
				break ;

				case xsd_length:
					xsdrest->m_length = getIntAttr(child,"value");
				break ;

				case xsd_minLength:
					xsdrest->m_minLength = getIntAttr(child,"value");
				break ;

				case xsd_maxLength:
					xsdrest->m_maxLength = getIntAttr(child,"value");
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,rest);
				break ;
			}
		}
	}
	return xsdrest;
}

xsdList * ParseList(xmlNodePtr list,xsdType * parent,Symtab & st)
{
	xsd_keyword kw ;
	xsdList * xsdlist;
	xsdSimpleType * simpletype = NULL;
	const char * itemtypename = NULL;
	for_each_attr(attr,list)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_itemType:
				itemtypename = getContent(attr->children);
			break ;

			default:
				not_supported_error(attr,list);
			break ;
		}
	}
	for_each_child(child,list)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_simpleType:
					if (itemtypename == NULL)
					{
						simpletype = ParseSimpleType(child,"",parent,st);
					}
					else
					{
						not_allowed_error(child,list);
					}
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,list);
				break ;
			}
		}
	}
	xsdlist = new xsdList(itemtypename,simpletype,parent) ;
	return xsdlist;
}

xsdUnion * ParseUnion(xmlNodePtr type,xsdType * parent,Symtab & st)
{
	const char * xsdmemberTypes = "" ;
	xsd_keyword kw ;
	xsdUnion   * xsdtype = NULL;
	for_each_attr(attr,type)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_itemType:
				xsdmemberTypes = getContent(attr->children);
			break ;
			default:
				not_supported_error(attr,type);
			break ;
		}
	}
	xsdtype = new xsdUnion(xsdmemberTypes,parent);
	for_each_child(child,type)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case xsd_simpleType:
					xsdtype->m_types.push_back(ParseSimpleType(child,"",parent,st));
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,type);
				break ;
			}
		}
	}
	return xsdtype;
}

xsdSimpleType * ParseSimpleType(xmlNodePtr type,const char * elemname,xsdType * parent,Symtab & st)
{
	xsdSimpleType * xsdtype = NULL;
	const char * xsdtypename = "" ;
	xsd_keyword kw ;
	for_each_attr(attr,type)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdtypename = getContent(attr->children);
				if (parent != NULL)
				{
					not_allowed_error(attr,type);
				}
			break ;

			default:
				not_supported_error(attr,type);
			break ;
		}
	}
	xsdtype = new xsdSimpleType(xsdtypename,elemname,parent);
	for_each_child(child,type)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xsdtype->m_rest  = ParseRestriction(child,xsdtype,st);
				break ;

				case	xsd_list:
					xsdtype->m_list  = ParseList(child,xsdtype,st);
				break ;

				case	xsd_union:
					xsdtype->m_union = ParseUnion(child,xsdtype,st);
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,type);
				break;
			}
		}
	}
	return xsdtype;
}

xsdSchema * ParseSchema(xmlNodePtr schema,Symtab & symtab);

bool fileexists(const char * filename)
{
	struct stat st ;
	return stat(filename,&st) == 0;
}

void DoImport(const char * filename,Symtab & symtab)
{
	xmlDocPtr doc = xmlReadFile(filename,"utf-8",0);
	if (doc != NULL)
	{
		for_each_child(child,doc)
		{
			if (child->type == XML_ELEMENT_NODE)
			{
				xsd_keyword kw = Lookup(child->name);
				switch(kw)
				{
					case	xsd_schema:
						ParseSchema(child,symtab);
					break ;

					case	xsd_annotation:
					break ;

					default:
					break ;
				}
			}
		}
	}
}

void ParseImport(xmlNodePtr import,Symtab & symtab)
{
	xsd_keyword kw ;
	for_each_attr(attr,import)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_schemaLocation:
			{
				char filename[256];
				bool found = false ;
				std::list<std::string*>::iterator si ;
				const char * value = getContent(attr->children);
				const char * cp = strrchr(value,'/');
				if (cp != NULL)
					cp++;
				else
					cp = value;
				strcpy(filename,cp);
				if (fileexists(filename))
				{
					found = true;
					DoImport(filename,symtab);
				}
				else
				{
					for (si = includePaths.begin() ; si != includePaths.end() ; si++)
					{
						std::string * s = *si ;
						strcpy(filename,s->c_str());
						strcat(filename,"/");
						strcat(filename,cp);
						if (fileexists(filename))
						{
							found = true ;
							DoImport(filename,symtab);
							break ;
						}
					}
					if (!found)
					{
						printf("can't open import \"%s\"\n",filename);
						printf("download \"%s\" and install locally\n",getContent(attr->children));
					}
				}
			}
			break ;

			default:
			break ;
		}
	}
}

xsdSchema * ParseSchema(xmlNodePtr schema,Symtab & st)
{
	xsdSchema * xmlschema = new xsdSchema();
	xsd_keyword kw;

	for_each_nsdef(nsdef,schema)
	{
		if (nsdef->type == XML_NAMESPACE_DECL && nsdef->prefix != NULL)
		{
			printf("new namespace prefix:\"%s\" href=\"%s\"\n",nsdef->prefix,nsdef->href);
			AddNamespace((const char*)nsdef->prefix,(const char*)nsdef->href);
		}
	}

	for_each_attr(attr,schema)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_targetNamespace:
				targetNamespace = FindNamespaceRef(getContent(attr->children));
			break ;
			case	xsd_xmlns:
			break ;
			default:
			break ;
		}
	}
	for_each_child(child,schema)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_import:
				{
					xsdNamespace * ns = targetNamespace;
					ParseImport(child,st);
					targetNamespace = ns ;
				}
				break ;

				case	xsd_element:
					AddElement(ParseElement(child,NULL,st));
				break ;

				case	xsd_complexType:
					AddType(ParseComplexType(child,NULL,NULL,st));
				break ;

				case	xsd_simpleType:
					AddType(ParseSimpleType(child,"",NULL,st));
				break ;

				case	xsd_annotation:
				break ;

				default:
					not_supported_error(child,schema);
				break ;
			}
		}
	}
	return xmlschema;
}

const char * xsdType::getCppName()
{
	const char * name ;
	switch(m_tag)
	{
		case  type_float:					     name = "xs_float";								break ;
		case	type_double:				     name = "xs_double";							break ;
		case	type_unsignedByte: 	     name = "xs_unsignedByte";				break ;
		case  type_unsignedShort:	     name = "xs_unsignedShort";				break ;
		case  type_nonNegativeInteger: name = "xs_nonNegativeInteger"; 	break ;
		case	type_positiveInteger:    name = "xs_positiveInteger";    	break ;
		case	type_unsignedInt: 	     name = "xs_unsignedInt";	       	break ;
		case	type_unsignedLong:       name = "xs_unsignedLong";    		break ;
		case	type_byte:    			     name = "xs_byte";				    		break ;
		case	type_short:					     name = "xs_short";			    			break ;
		case	type_negativeInteger:		 name = "xs_negativeInteger";     break ;
		case	type_int:						     name = "xs_int";									break ;
		case	type_long:    			     name = "xs_long";								break ;
		case	type_boolean:				     name = "xs_boolean";								break ;
		case  type_string:             name = "xs_string";							break ;
		case	type_integer:            name = "xs_integer"	;						break ;
		case	type_dateTime:           name = "xs_dateTime";            break ;
		case  type_all:
		case	type_enum:
		case	type_restriction:
		case	type_extension:
		case	type_simpleContent:
		case	type_list:
		case	type_union:
		case	type_simple:
		case	type_complex:
		case	type_sequence:
		case	type_choice:
			if (m_cname.empty() && m_parent != NULL)
				name = m_parent->getCppName();
			else
			  name = m_cname.c_str();
		break ;

		default:               		name = m_cname.c_str();	break ;
	}
	return name ;
}

const char * xsdType::getNativeName()
{
	const char * name ;
	switch(m_tag)
	{
		case  type_float:					     name = "float";								break ;
		case	type_double:				     name = "double";							break ;
		case	type_unsignedByte: 	     name = "unsigned char";				break ;
		case  type_unsignedShort:	     name = "unsigned short";				break ;
		case  type_nonNegativeInteger: name = "unsigned int"; 	break ;
		case	type_positiveInteger:    name = "unsigned int";    	break ;
		case	type_unsignedInt: 	     name = "unsigned int";	       	break ;
		case	type_unsignedLong:       name = "unsigned Long";    		break ;
		case	type_byte:    			     name = "unsigned char";				    		break ;
		case	type_short:					     name = "short";			    			break ;
		case	type_negativeInteger:		 name = "int";     break ;
		case	type_int:						     name = "int";									break ;
		case	type_long:    			     name = "long";								break ;
		case	type_boolean:				     name = "bool";								break ;
		case	type_integer:            name = "int"	;						break ;

		default:
			name =getCppName();
		break ;
	}
	return name ;
}

bool  xsdType::isUnsignedInteger()
{
	bool res ;
	switch(m_tag)
	{
		case	type_unsignedByte:
		case  type_unsignedShort:
		case	type_unsignedInt:
		case	type_unsignedLong:
		case  type_nonNegativeInteger:
		case	type_positiveInteger:
			res = true;
		break ;

		default:
			res = false;
		break ;
	}
	return res ;
}

bool  xsdType::isSignedInteger()
{
	bool res ;
	switch(m_tag)
	{
		case	type_byte:
		case  type_short:
		case	type_int:
		case	type_long:
		case  type_negativeInteger:
		case	type_integer:
			res = true;
		break ;

		default:
			res = false;
		break ;
	}
	return res ;
}

bool	xsdType::isInteger()
{
	return isUnsignedInteger() || isSignedInteger() ;
}

bool  xsdType::isfloat()
{
	bool res ;
	switch(m_tag)
	{
		case  type_float:
		case	type_double:
			res = true;
		break ;

		default:
			res = false;
		break ;
	}
	return res ;
}

bool  xsdType::isScalar()
{
	return isInteger() || isfloat() ;
}

bool  xsdType::isString()
{
	return m_tag == type_string;
}

std::string xsdType::getQualifiedName()
{
	std::string res ;
	if (m_parent == NULL)
	{
		res = m_cname;
#if DEBUG
		printf("QualifiedName=%s\n",res.c_str());
#endif
	}
	else
	{
		res = m_parent->getQualifiedName();
		if (!m_cname.empty())
		{
			res += "::" ;
			res += m_cname;
#if DEBUG
		printf("QualifiedName=%s\n",res.c_str());
#endif
		}
	}
	return res ;
}

int xsdTypeList::nextlistno;

void xsdTypeList::CalcDependency(xsdTypeList & list)
{
	for (typeIterator ti = begin() ; ti != end() ; ti++)
	{
		xsdType * type = *ti;
		type->CalcDependency(list);
	}
}

void xsdGroup::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	if (m_type != NULL)
	{
		m_type->CalcDependency(list);
	}
}

void xsdGroupList::CalcDependency(xsdTypeList & list)
{
	for(groupIterator gi = begin() ; gi != end() ; gi++)
	{
		(*gi)->CalcDependency(list);
	}
}

void xsdSimpleContent::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	if (m_content != NULL)
	{
		m_content->CalcDependency(list);
	}
}

void xsdList::CalcDependency(xsdTypeList & list)
{
	if (m_itemtypename != NULL)
		m_itemtype = FindType(m_itemtypename);
	if (m_itemtype != NULL)
		m_itemtype->CalcDependency(list);
}

void xsdRestriction::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	if (m_base != NULL)
		m_base->CalcDependency(list);
	if (m_simple != NULL)
		m_simple->CalcDependency(list);
}

#if 1
void xsdAttribute::CalcDependency(xsdTypeList & list)
{
	if (m_type != NULL)
		m_type->CalcDependency(list);
}
#endif

void xsdExtension::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	if (m_base != NULL)
	{
		m_base->CalcDependency(list);
		for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
		{
			xsdAttribute * attr = *ai;
			attr->CalcDependency(list);
		}
	}
}

void xsdSequenceList::CalcDependency(xsdTypeList & list)
{
	for (sequenceIterator si = begin() ; si != end() ; si++)
	{
		(*si)->CalcDependency(list);
	}
}

void xsdSequence::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	;
	m_elements.CalcDependency(list);
	m_types.CalcDependency(list);
}

void xsdAll::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	;
	m_elements.CalcDependency(list);
}

bool xsdAll::CheckCycle(xsdElement * elem)
{
	return m_elements.CheckCycle(elem);
}


void xsdChoice::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	;
	m_elements.CalcDependency(list);
	m_sequences.CalcDependency(list);
	m_groups.CalcDependency(list);
}

bool xsdChoice::CheckCycle(xsdElement * elem)
{
	return m_elements.CheckCycle(elem);
}

void xsdComplexType::CalcDependency(xsdTypeList & list)
{
	if (tascd())
	{
		return ;
	}
	;
	if (!m_indeplist)
	{
		for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
		{
			(*ai)->CalcDependency(list);
		}
		if (m_type != NULL)
		{
			m_type->CalcDependency(list);
		}
		if (m_parent == NULL)
		{
			m_indeplist = true;
#if DEBUG
			printf("add %s to deplist %d\n",getCppName(),list.listno);
#endif
			list.push_back(this);
		}
	}
}
bool xsdComplexType::CheckCycle(xsdElement * elem)
{
	if (m_type != NULL)
		return m_type->CheckCycle(elem);
	return false ;
}

void xsdSimpleType::CalcDependency(xsdTypeList & list)
{
	if (tascd())
		return ;
	;
	if (!m_indeplist)
	{
		if (m_rest != NULL)
			m_rest->CalcDependency(list);
		else if (m_list != NULL)
			m_list->CalcDependency(list);
		else if (m_union != NULL)
			m_union->CalcDependency(list);
		if (m_parent == NULL)
		{
			m_indeplist = true ;
#if DEBUG
			printf("add %s to deplist %d\n",getCppName(),list.listno);
#endif
			list.push_back(this);
		}
	}
}


void xsdElement::CalcDependency(xsdTypeList & list)
{
#if DEBUG
	printf("CalcDep for elem %s\n",getCppName());
#endif
	if (m_type == NULL)
	{
		if (m_typename == NULL)
		{
			m_typename = new xsdTypename("xs:string");
		}
		if (m_typename != NULL)
		{
			m_type = FindType(m_typename);
			if (m_type == NULL)
			{
				printf("element \"%s\": type  \"%s\" not found\n",getName(),m_typename->m_name.c_str());
			}
		}
	}
	if (m_type != NULL)
	{
		if (m_type->isLocal() && !m_type->m_indeplist)
		{
			m_type->CalcDependency(m_deplist);
			if (!m_type->m_indeplist)
			{
#if DEBUG
				printf("add %s to %s deplist %d\n",m_type->getCppName(),getCppName(),m_deplist.listno);
#endif
				m_deplist.push_back(m_type);
			}
		}
		else
		{
			m_isCyclic = m_type->CheckCycle(this);
			if (!m_isCyclic)
				m_type->CalcDependency(m_tns->m_deplist);
			else
				printf("%s cyclic ref to %s\n",getName(),m_type->getName());
		}
	}
}

bool xsdElement::hasAttributes()
{
	bool res = false ;
	if (m_type != NULL)
		res = m_type->hasAttributes();
	return res ;
}

void xsdElementList::CalcDependency(xsdTypeList & list)
{
	elementIterator eli ;
	for (eli = begin() ; eli != end(); eli++)
	{
		xsdElement * elem = *eli ;
		elem->CalcDependency(list);
	}
}

bool xsdElementList::CheckCycle(xsdElement * elem)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		if (elem == *ei)
			return true ;
	}
	return false ;
}



int main(int argc, char * argv[])
{
	const char * xsdfilename = NULL;
	char hfilename[256];
	char cppfilename[256];
	for (int i = 1 ; i < argc ; i++)
	{
		const char * arg = argv[i];
		if (arg[0] == '-')
		{
			switch(arg[1])
			{
				case	'I':
					if (arg[2])
						AddIncludePath(arg+2); // -Iincludepath
					else
					{
						i++;
						if (i < argc)
						{
							AddIncludePath(argv[i]); // -I includepath
						}
					}
				break ;

				default:
					printf("unkown option \"%s\"\n",arg);
				break ;
			}
		}
		else
		{
			if (xsdfilename == NULL)
				xsdfilename = argv[i];
			else
				printf("only one inputfile allowed\n");
		}
	}
	if (xsdfilename == NULL)
	{
		printf("no inputfile\n");
		exit(1);
	}
	strcpy(hfilename,xsdfilename);
	char * cp = strrchr(hfilename,'.');
	if (cp != NULL)
		strcpy(cp,".h");
	else
		strcat(hfilename,".h");
	strcpy(cppfilename,hfilename);
	cp = strrchr(cppfilename,'.');
	if (cp != NULL)
		strcpy(cp,".cpp");
	xmlDocPtr doc = xmlReadFile(xsdfilename,"utf-8",0);
	if (doc != NULL)
	{
		CppFile hfile(hfilename) ;
		if (hfile.create() == false)
		{
			printf("can't create %s - %s\n",hfilename,strerror(errno));
			exit(2);
		}
		hfile.printf("/*\n automatic created by xsdclassgen\n*/\n\n");
		std::string hfiledefine ;
		std::string hfileinclude;
		cp = strrchr(hfilename,'/');
		if (cp == NULL)
			cp = strrchr(hfilename,'\\');
		if (cp == NULL)
			cp = hfilename;
		else
			cp = cp + 1 ;
		hfileinclude = cp ;
		if (!isalpha(*cp) && *cp != '_')
		{
			hfiledefine += "_";
			cp++;
		}
		while(*cp)
		{
			char c = toupper(*cp++);
			if (!isalnum(c) && c != '_')
				c = '_' ;
			hfiledefine += c ;
		}
		hfiledefine += "_INCLUDED";
		hfile.println("#ifndef %s",hfiledefine.c_str());
		hfile.println("#define %s\n",hfiledefine.c_str());
		CppFile cppfile(cppfilename);
		if (cppfile.create() == false)
		{
			printf("can't create %s - %s\n",cppfilename,strerror(errno));
			exit(2);
		}
		hfile.println("#include <stdio.h>");
		hfile.println("#include <stdlib.h>");
		hfile.println("#include <string.h>");
		hfile.println("#include <libxml/parser.h>");
		hfile.println("#include \"xsdtypes.h\"");
		hfile.println();
		cppfile.println("#include \"%s\"",hfileinclude.c_str());
		cppfile.println();
		cppfile.println("#define for_each_child(child,node) for (xmlNodePtr child = node->children   ; child != NULL ; child = child->next)");
		cppfile.println("#define for_each_attr(attr,node)   for (xmlAttrPtr attr  = node->properties ; attr != NULL  ; attr = attr->next)");
		cppfile.println();


		AddNamespace("xs","http://www.w3.org/2009/xml.xsd");
		AddType(new xsdType("xs:int",type_int,NULL,true));
		AddType(new xsdType("xs:string",type_string,NULL,true));
		AddType(new xsdType("xs:boolean",type_boolean,NULL,true));
		AddType(new xsdType("xs:decimal",type_decimal,NULL,true));
		AddType(new xsdType("xs:float",type_float,NULL,true));
		AddType(new xsdType("xs:double",type_double,NULL,true));
		AddType(new xsdType("xs:integer",type_integer,NULL,true));
		AddType(new xsdType("xs:long",type_long,NULL,true));
		AddType(new xsdType("xs:short",type_short,NULL,true));
		AddType(new xsdType("xs:byte",type_byte,NULL,true));
		AddType(new xsdType("xs:nonNegativeInteger",type_nonNegativeInteger,NULL,true));
		AddType(new xsdType("xs:unsignedLong",type_unsignedLong,NULL,true));
		AddType(new xsdType("xs:unsignedInt",type_unsignedInt,NULL,true));
		AddType(new xsdType("xs:unsignedShort",type_unsignedShort,NULL,true));
		AddType(new xsdType("xs:unsignedByte",type_unsignedByte,NULL,true));
		AddType(new xsdType("xs:unsignedPositiveInteger",type_positiveInteger,NULL,true));
		AddType(new xsdType("xs:dateTime",type_dateTime,NULL,true));
		Symtab symtab ;
		for_each_child(child,doc)
		{
			if (child->type == XML_ELEMENT_NODE)
			{
				xsd_keyword kw = Lookup(child->name);
				switch(kw)
				{
					case	xsd_schema:
						ParseSchema(child,symtab);
					break ;

					default:
						not_supported_error(child,doc);
					break ;
				}
			}
		}
		symtab.sortbyname();
		cppfile.GenSymtab(symtab);
		for (NamespaceList::iterator nsi = namespaces.begin() ; nsi != namespaces.end() ; nsi++)
		{
			xsdNamespace * ns = *nsi ;
#if DEBUG
			printf("global typelist %d\n",ns->m_deplist.listno);
#endif
			ns->m_types.CalcDependency(ns->m_deplist);
			for (typeIterator ti = ns->m_deplist.begin() ; ti != ns->m_deplist.end() ; ti++)
			{
				xsdType * type = *ti;
#if DEBUG
				printf("type %s from deplist\n",type->getCppName());
#endif
				type->GenHeader(hfile,0,NULL);
				type->GenLocal(cppfile,symtab,NULL);
				type->GenImpl(cppfile,symtab,NULL);
			}
		}
#if 1
		for (NamespaceList::iterator nsi = namespaces.begin() ; nsi != namespaces.end() ; nsi++)
		{
			xsdNamespace * ns = *nsi ;
			elementIterator ei ;
			for (ei = ns->m_elements.begin() ; ei != ns->m_elements.end(); ei++)
			{
				xsdElement * elem = *ei ;
				elem->CalcDependency(elem->m_deplist);
				if (elem->m_type != NULL)
				{
					elem->m_type->GenHeader(hfile,0,elem->getDefault());
					elem->m_type->GenImpl(cppfile,symtab,elem->getDefault());
					elem->GenRootHeader(hfile,0);
					elem->GenRootImpl(cppfile,symtab);
				}
			}
		}
#endif
		hfile.printf("#endif // %s\n",hfiledefine.c_str());
		hfile.close();
		cppfile.close();
	}
	return 0 ;
}

bool xsdGroup::CheckCycle(xsdElement* elem)
{
	return m_type->CheckCycle(elem);
}

void xsdTypeList::GenLocal(CppFile& out, Symtab& st, const char* defaultstr)
{
}

void xsdAttrList::GenHeader(CppFile& out, int indent, const char* defaultstr)
{
	for (attrIterator ai = begin() ; ai != end() ; ai++)
	{
		xsdAttribute * attr = *ai;
		attr->GenHeader(out,indent);
	}
}

bool xsdSequence::CheckCycle(xsdElement* elem)
{
	return m_elements.CheckCycle(elem);
}

void xsdGroupList::GenHeader(CppFile& out, int indent, const char* defaultstr)
{
}

void xsdComplexContent::CalcDependency(xsdTypeList& list)
{
	if (tascd())
		return ;
	;
	if (m_type != NULL)
		m_type->CalcDependency(list);
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


