/*
 * xsdclassgen.cpp
 *
 *  Created on: 26.11.2012
 *      Author: dorwig
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <libxml2/libxml/parser.h>
#include "xsdclassgen.h"

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
  xsd_fractionDigits,
  xsd_group,
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
  xsd_pattern,
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
    ENTRY(fractionDigits),
    ENTRY(group),
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
    ENTRY(pattern),
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
			if (!type->isAnonym() && type->m_name == name)
				return type ;
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
} ;

typedef std::list<xsdNamespace*> NamespaceList ;
NamespaceList namespaces;
xsdNamespace *targetNamespace;

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
	xsdNamespace * ns = targetNamespace;
	if (!elem->m_ns.empty())
		ns = FindNamespace(elem->m_ns.c_str());
	if (ns != NULL)
		ns->AddElement(elem);
}

void not_allowed_error(xmlNodePtr e,xmlNodePtr n)
{
	printf("\"%s\" not allowed in \"%s\"\n",e->name,n->name);
}

void not_supported_error(xmlNodePtr e,xmlNodePtr n)
{
	printf("\"%s\" not supported in \"%s\"\n",e->name,n->name);
}

void not_supported_attribute_error(xmlAttrPtr a,xmlNodePtr n)
{
	printf("\"%s\" not supported in \"%s\"\n",a->name,n->name);
}

xsdRestriction * ParseRestriction(xmlNodePtr rest);
xsdComplexType * ParseComplexType(xmlNodePtr type);
xsdSimpleType  * ParseSimpleType(xmlNodePtr type);
xsdChoice      * ParseChoice(xmlNodePtr choice);

xsdAttribute   * ParseAttribute(xmlNodePtr node)
{
	xsd_keyword kw ;
	const char * xsdname = "";
	const char * xsddefault = "";
	xsdType * xsdtype = NULL;

	for_each_attr(attr,node)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdname = getContent(attr->children);
			break ;

			case	xsd_default:
				xsddefault = getContent(attr->children);
			break ;

			case	xsd_type:
				xsdtype = FindType(getContent(attr->children));
			break ;
			default:
			break ;
		}
	}
	xsdAttribute * xsdattr = new xsdAttribute(xsdname,xsddefault,xsdtype);
	return xsdattr;
}

xsdElement * ParseElement(xmlNodePtr element)
{
	xsdElement * xsdelem     = NULL;
	xsdType    * xsdtype     = NULL;
	const char * xsdname     = NULL;
	xsdTypename* xsdtypename = NULL;
	int minOccurs = 0 ;
	int maxOccurs = 0 ;

	for_each_attr(attr,element)
	{
		xsd_keyword kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdname = getContent(attr->children);
			break ;

			case	xsd_type:
				xsdtypename = new xsdTypename(getContent(attr->children));
			break ;

			case	xsd_minOccurs:
				minOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			case	xsd_maxOccurs:
				maxOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			default:
				not_supported_attribute_error(attr,element);
			break ;
		}
	}
	for_each_child(child,element)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			xsd_keyword kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_complexType:
					if (xsdtype != NULL)
						printf("element %s already has a type %s\n",xsdname,xsdtype->getName());
					else
						xsdtype = ParseComplexType(child);
				break ;

				case	xsd_simpleType:
					if (xsdtype != NULL)
						printf("element %s already has a type %s\n",xsdname,xsdtype->getCppName());
					else
						xsdtype = ParseSimpleType(child);
				break ;

				default:
					not_supported_error(child,element);
				break ;
			}
		}
	}
	xsdelem = new xsdElement(xsdname,xsdtypename,xsdtype,minOccurs,maxOccurs);
	return xsdelem;
}

xsdSequence * ParseSequence(xmlNodePtr sequence)
{
	xsd_keyword kw ;
	xsdSequence * xmlseq = new xsdSequence();
	for_each_child(child,sequence)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_element:
					xmlseq->m_list.push_back(ParseElement(child));
				break ;

				default:
					not_supported_error(child,sequence);
				break ;
			}
		}
	}
	return xmlseq;
}

xsdAll * ParseAll(xmlNodePtr all)
{
	xsdAll * xsdtype ;
	xsd_keyword kw ;
	for_each_attr(attr,all)
	{

	}
	xsdtype = new xsdAll();
	for_each_child(child,all)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_element:
					xsdtype->m_elements.push_back(ParseElement(child));
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

xsdGroup  * ParseGroup(xmlNodePtr group)
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
				not_supported_attribute_error(attr,group);
			break ;
		}
	}
	xsdgroup = new xsdGroup(xsdname) ;

	for_each_child(child,group)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_all:
					xsdgroup->m_type = ParseAll(child);
				break ;

				case	xsd_choice:
					xsdgroup->m_type = ParseChoice(child);
				break ;

				case	xsd_sequence:
					xsdgroup->m_type = ParseSequence(child);
				break ;

				default:
					not_supported_error(child,group);
				break ;
			}
		}
	}
	return xsdgroup;
}

xsdChoice * ParseChoice(xmlNodePtr choice)
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
				not_supported_attribute_error(attr,choice);
			break ;
		}
	}
	xsdChoice * xsdchoice = new xsdChoice(min,max);
	for_each_child(child,choice)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_element:
					xsdchoice->m_elements.push_back(ParseElement(child));
				break ;


				case	xsd_choice:
					xsdchoice->m_choises.push_back(ParseChoice(child));
				break ;

				case	xsd_sequence:
					xsdchoice->m_sequences.push_back(ParseSequence(child));
				break ;

				case	xsd_group:
					xsdchoice->m_groups.push_back(ParseGroup(child));
				break ;

				case	xsd_any:
				default:
					not_supported_error(child,choice);
				break ;
			}
		}
	}

	return xsdchoice;
}

xsdExtension * ParseExtension(xmlNodePtr ext)
{
	xsd_keyword kw ;
	xsdExtension * xsdext = NULL;
	xsdTypename  * basetypename = NULL;
	for_each_attr(attr,ext)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_base:
				basetypename = new xsdTypename(getContent(attr->children));
			break ;
			default:
			break ;
		}
	}
	xsdext = new xsdExtension(basetypename);
	for_each_child(child,ext)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_attribute:
					xsdext->m_attributes.push_back(ParseAttribute(child));
				break ;

				default:
					not_supported_error(child,ext);
				break ;
			}
		}
	}
	return xsdext;
}

xsdSimpleContent * ParseSimpleContent(xmlNodePtr cont)
{
	xsdSimpleContent * xsdcontent = new xsdSimpleContent();
	xsd_keyword kw ;
	for_each_child(child,cont)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xsdcontent->m_content = ParseRestriction(child);
				break ;

				case	xsd_extension:
					xsdcontent->m_content = ParseExtension(child);
				break ;

				default:
					not_supported_error(child,cont);
				break ;
			}
		}
	}
	return xsdcontent;
}

xsdComplexType * ParseComplexType(xmlNodePtr type)
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
	xsdComplexType * newtype = new xsdComplexType(xsdtypename,type_complex);
	for_each_child(child,type)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name) ;
			switch(kw)
			{
				case  xsd_attribute:
					newtype->m_attributes.push_back(ParseAttribute(child));
				break ;

				case	xsd_sequence:
					newtype->m_type = ParseSequence(child);
				break ;

				case	xsd_choice:
					newtype->m_type = ParseChoice(child);
				break ;

				case	xsd_all:
					newtype->m_type = ParseAll(child);
				break ;

				case	xsd_simpleContent:
					newtype->m_type = ParseSimpleContent(child);
				break ;

				default:
					not_supported_error(child,type);
				break ;
			}
		}
	}
	return newtype;
}

xsdEnumValue * ParseEnum(xmlNodePtr ptr)
{
	xsd_keyword kw ;
	for_each_attr(attr,ptr)
	{
		kw = Lookup(attr->name);
		if (kw == xsd_value)
		{
			return new xsdEnumValue(getContent(attr->children));
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

xsdRestriction * ParseRestriction(xmlNodePtr rest)
{
	xsdRestriction * xmlrest = new xsdRestriction();
	xsd_keyword kw ;
	for_each_attr(attr,rest)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_base:
				xmlrest->m_base = FindType(getContent(attr->children));
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
				case	xsd_enumeration:
					xmlrest->m_enumvalues.push_back(ParseEnum(child));
				break ;

				case	xsd_minExclusive:
					xmlrest->m_minExclusive = getIntAttr(child,"value");
				break ;

				case	xsd_maxExclusive:
					xmlrest->m_maxExclusive = getIntAttr(child,"value");;
				break ;

				case	xsd_minInclusive:
					xmlrest->m_minInclusive = getIntAttr(child,"value");;
				break ;

				case	xsd_maxInclusive:
					xmlrest->m_maxInclusive = getIntAttr(child,"value");;
				break ;

				case	xsd_totalDigits:
					xmlrest->m_totalDigits = getIntAttr(child,"value");
				break ;

				case	xsd_fractionDigits:
					xmlrest->m_fractionDigits= getIntAttr(child,"value");
				break ;

				case xsd_length:
					xmlrest->m_length = getIntAttr(child,"value");
				break ;

				case xsd_minLength:
					xmlrest->m_minLength = getIntAttr(child,"value");
				break ;

				case xsd_maxLength:
					xmlrest->m_maxLength = getIntAttr(child,"value");
				break ;

				default:
				break ;
			}
		}
	}
	return xmlrest;
}

xsdList * ParseList(xmlNodePtr list)
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
						simpletype = ParseSimpleType(child);
					}
					else
					{
						not_allowed_error(child,list);
					}
				break ;

				default:
					not_supported_error(child,list);
				break ;
			}
		}
	}
	xsdlist = new xsdList(itemtypename,simpletype) ;
	return xsdlist;
}

xsdSimpleType * ParseSimpleType(xmlNodePtr type)
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
			break ;
			default:
			break ;
		}
	}
	xsdtype = new xsdSimpleType(xsdtypename);
	for_each_child(child,type)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xsdtype->m_derived = ParseRestriction(child);
				break ;

				case	xsd_list:
					xsdtype->m_derived = ParseList(child);
				break ;

				default:
					not_supported_error(child,type);
				break;
			}
		}
	}
	return xsdtype;
}

xsdSchema * ParseSchema(xmlNodePtr schema);

void ParseImport(xmlNodePtr import)
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
				for (si = includePaths.begin() ; si != includePaths.end() ; si++)
				{
					struct stat st ;
					std::string * s = *si ;
					strcpy(filename,s->c_str());
					strcat(filename,"/");
					strcat(filename,cp);
					if (stat(filename,&st) == -1)
					{
						continue ;
					}
					found = true ;
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
										ParseSchema(child);
									break ;

									default:
									break ;
								}
							}
						}
					}
				}
				if (!found)
				{
					printf("can't open import \"%s\"\n",filename);
					printf("download \"%s\" and install locally\n",getContent(attr->children));
				}
			}
			break ;

			default:
			break ;
		}
	}
}

xsdSchema * ParseSchema(xmlNodePtr schema)
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
		printf("Schema attr %s %s\n",attr->name,getContent(attr->children));
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
					ParseImport(child);
					targetNamespace = ns ;
				}
				break ;

				case	xsd_element:
					AddElement(ParseElement(child));
				break ;

				case	xsd_complexType:
					AddType(ParseComplexType(child));
				break ;

				case	xsd_simpleType:
					AddType(ParseSimpleType(child));
				break ;

				default:
					not_supported_error(child,schema);
				break ;
			}
		}
	}
	return xmlschema;
}

void Indent(FILE * out,int indent)
{
	while(--indent >= 0)
	{
		fputs("  ",out); // 2 Blanks
		fflush(out);
	}
}

void xsdType::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	const char * name = getCppName();
	if (elemname)
	{
		Indent(out,indent);
		fprintf(out,"%s %s;\n",name,elemname);
	}
}

void xsdType::GenCode(FILE * out,int indent)
{

}

const char * xsdType::getCppName()
{
	const char * name ;
	switch(m_tag)
	{
		case  type_float:					name = "float";					break ;
		case	type_double:				name = "double";				break ;
		case	type_unsignedByte: 	name = "unsigned char";	break ;
		case  type_unsignedShort:	name = "unsigned short";break ;
		case	type_unsignedInt: 	name = "unsigned int";	break ;
		case	type_unsignedLong:  name = "unsigned long"; break ;
		case	type_byte:    			name = "char";					break ;
		case	type_short:					name = "short";					break ;
		case	type_int:						name = "int";						break ;
		case	type_long:    			name = "long";					break ;
		case	type_boolean:				name = "bool";					break ;
		case  type_string:        name = "char";   				break ;
		default:               		name = m_cname.c_str();	break ;
	}
	return name ;
}

void xsdRestriction::GenCode(FILE * out,int indent,const char * elemname,const char * simplename)
{
	if (!m_enumvalues.empty())
	{
		Indent(out,indent);
		fprintf(out,"enum %s\n",simplename);
		Indent(out,indent);
		fprintf(out,"{\n");
		std::list<xsdEnumValue*>::iterator si ;
		for (si = m_enumvalues.begin() ; si != m_enumvalues.end() ; si++)
		{
			Indent(out,indent+1);
			fprintf(out,"%s_%s,\n",simplename,(*si)->getCppName());
		}
		Indent(out,indent);
		fprintf(out,"} %s ;\n",elemname);
	}
	else
	{
		if (m_base != NULL)
		{
			int len = m_length;
			if (m_maxLength)
				len = m_maxLength;
			Indent(out,indent);
			if (*elemname == 0)
			{
				if (len > 1)
					fprintf(out,"typedef %s %s[%d];\n",m_base->getCppName(),simplename,len);
				else
					fprintf(out,"typedef %s %s;\n",m_base->getCppName(),simplename);
			}
			else
			{
				if (len > 1)
					fprintf(out,"%s %s[%d];\n",m_base->getCppName(),elemname,len);
				else
					fprintf(out,"%s %s;\n",m_base->getCppName(),elemname);
			}
		}
	}
}

void xsdList::GenCode(FILE *out,int indent,const char * elemname,const char * simplename)
{
	xsdType * type = m_type;
	if (m_typename != NULL)
	{
		type = FindType(m_typename);
	}
	if (type != NULL)
	{
		Indent(out,indent);
		fprintf(out,"typedef %s %s;\n",type->getCppName(),simplename);
	}
}

void xsdSimpleType::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_derived != NULL)
			m_derived->GenCode(out,indent,elemname,getCppName());
		else
			xsdType::GenCode(out,indent,elemname,supertypename);
	}
}


void xsdSequence::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	std::list<xsdElement*>::iterator ei;
	Indent(out,indent);
	fprintf(out,"struct %s\n",supertypename);
	Indent(out,indent);
	fprintf(out,"{\n");
	indent++;
	GenCode(out,indent);
	indent--;
	Indent(out,indent);
	fprintf(out,"} %s;\n\n",elemname == NULL ? "" : elemname);
}

void xsdSequence::GenCode(FILE * out,int indent)
{
	m_list.GenCode(out,indent,false);
}

void xsdAll::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	std::list<xsdElement*>::iterator ei;
	Indent(out,indent);
	fprintf(out,"struct %s\n",supertypename);
	Indent(out,indent);
	fprintf(out,"{\n");
	indent++;
	GenCode(out,indent);
	indent--;
	Indent(out,indent);
	fprintf(out,"} %s;\n\n",elemname == NULL ? "" : elemname);
}

void xsdAll::GenCode(FILE * out,int indent)
{
	m_elements.GenCode(out,indent,false);
}

void xsdExtension::GenCode(FILE *out,int indent,const char * elemname,const char * supertypename)
{
	Indent(out,indent);
	fprintf(out,"struct %s\n",supertypename);
	Indent(out,indent);
	fprintf(out,"{\n");
	for (xsdAttrList::iterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
	{
		xsdAttribute * attr = *ai ;
		attr->GenCode(out,indent+1);
	}
	Indent(out,indent+1);
	fprintf(out,"%s %s;\n",m_basetypename->getCppName(),"m_val");
	Indent(out,indent);
	fprintf(out,"} %s;\n",elemname);
}

void xsdSimpleContent::GenCode(FILE *out,int indent,const char * elemname,const char * supertypename)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_content != NULL)
		{
			m_content->GenCode(out,indent,elemname,supertypename);
		}
	}
}

void xsdChoice::GenCode(FILE *out,int indent,const char * elemname,const char * supertypename)
{
	GenCode(out,indent);
}

void xsdChoice::GenCode(FILE *out,int indent)
{
	if (!m_impl)
	{
		m_impl = true ;
		m_elements.GenCode(out,indent,true);
	}
}

void xsdComplexType::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	if (!m_impl)
	{
		if (!m_cname.empty())
			supertypename = getCppName();
		m_impl = true;
		if (!m_attributes.empty())
		{
			Indent(out,indent);
			fprintf(out,"struct %s\n",supertypename);
			Indent(out,indent);
			fprintf(out,"{\n");
			for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
			{
				xsdAttribute * attr = *ai;
				attr->GenCode(out,indent+1);
			}
			if (m_type != NULL)
				m_type->GenCode(out,indent+1);
			Indent(out,indent);
			fprintf(out,"} %s;\n",elemname);
		}
		else if (m_type != NULL)
		{
			m_type->GenCode(out,indent,elemname,supertypename);
		}
	}
}

void xsdElement::GenCode(FILE *out,int indent,bool choice)
{
	std::string cppName;
	if (choice)
		cppName = "* ";
	cppName += getCppName();
	if (m_type != NULL)
		m_type->GenCode(out,indent,cppName.c_str(),"");
	else
	{
		if (m_typename != NULL)
		{
			xsdType * type = FindType(m_typename);
			Indent(out,indent);
			if (type == NULL)
			{
				printf("Element %s has unknown type %s\n",getName(),m_typename->m_name.c_str());
				fprintf(out,"%s %s;\n",m_typename->m_name.c_str(),cppName.c_str());
			}
			else
			{
				fprintf(out,"%s %s;\n",type->getCppName(),cppName.c_str());
			}
		}
	}
}

void xsdGroup::CalcDependency(xsdTypeList & list)
{
	if (m_type != NULL)
	{
		m_type->CalcDependency(list);
	}
}

void xsdGroup::GenCode(FILE * out,int indent)
{
	if (m_type != NULL)
		m_type->GenCode(out,indent);
}

void xsdGroup::GenCode(FILE * out,int indent,const char * elemname,const char * supertypname)
{
	if (m_type != NULL)
	{
		m_type->GenCode(out,indent,elemname,supertypname);
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
	if (m_content != NULL)
	{
		m_content->CalcDependency(list);
	}
}

void xsdList::CalcDependency(xsdTypeList & list)
{
	xsdType * type = m_type ;
	if (type == NULL && m_typename != NULL)
		type = FindType(m_typename);
	if (type != NULL)
		type->CalcDependency(list);
}

void xsdRestriction::CalcDependency(xsdTypeList & list)
{
	if (m_base != NULL)
		m_base->CalcDependency(list);
}

void xsdAttribute::CalcDependency(xsdTypeList & list)
{
	if (m_type != NULL)
		m_type->CalcDependency(list);
}

void xsdAttribute::GenCode(FILE * out,int indent)
{
	if (m_type != NULL)
	{
		Indent(out,indent);
		fprintf(out,"%s %s;\n",m_type->getCppName(),m_cname.c_str());
	}
}

void xsdExtension::CalcDependency(xsdTypeList & list)
{
	if (m_basetypename != NULL)
	{
		xsdType * type = FindType(m_basetypename);
		if (type != NULL)
			type->CalcDependency(list);
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
	m_list.CalcDependency(list);
}

void xsdAll::CalcDependency(xsdTypeList & list)
{
	m_elements.CalcDependency(list);
}


void xsdChoice::CalcDependency(xsdTypeList & list)
{
	m_elements.CalcDependency(list);
	m_sequences.CalcDependency(list);
	m_groups.CalcDependency(list);
}

void xsdComplexType::CalcDependency(xsdTypeList & list)
{
	if (!m_indeplist)
	{
		m_indeplist = true;
		for (attrIterator ai = m_attributes.begin() ; ai != m_attributes.end() ; ai++)
		{
			(*ai)->CalcDependency(list);
		}
		if (m_type != NULL)
		{
			m_type->CalcDependency(list);
		}
		list.push_back(this);
	}
}

void xsdSimpleType::CalcDependency(xsdTypeList & list)
{
	if (m_name.empty())
		return ;
	if (m_derived != NULL && !m_indeplist)
	{
		m_derived->CalcDependency(list);
		m_indeplist = true ;
		list.push_back(this);
	}
}

void xsdElement::CalcDependency(xsdTypeList & list)
{
	xsdType * type = m_type;
	printf("CalcDependency for %s\n",getName());
	if (type == NULL)
	{
		if (m_typename != NULL)
		{
			type = FindType(m_typename);
			if (type == NULL)
			{
				printf("element \"%s\": type  \"%s\" not found\n",getName(),m_typename->m_name.c_str());
			}
		}
		else
		{
			printf("element \"%s\" has not type\n",getName());
		}
	}
	if (type != NULL)
	{
		type->CalcDependency(list);
	}
}

void xsdElementList::CalcDependency(xsdTypeList & list)
{
	elementIterator eli ;
	typeIterator tli ;
	for (eli = begin() ; eli != end(); eli++)
	{
		xsdElement * elem = *eli ;
		elem->CalcDependency(list);
	}
}

void xsdElementList::GenCode(FILE * out,int indent,bool choice)
{
	for (elementIterator ei = begin() ; ei != end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenCode(out,indent,choice);
	}
}

xsdType * createDateTime()
{
	xsdSimpleType * st = new xsdSimpleType("xs:dateTime");
	xsdRestriction *r = new xsdRestriction();
	r->m_base = FindType("xs:string");
	r->m_maxLength=20;
	st->m_derived = r ;
	return st ;
}

int main(int argc, char * argv[])
{
	xsdSchema * xmlschema = NULL;
	const char * xsdfilename = NULL;
	char hfilename[256];

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

	xmlDocPtr doc = xmlReadFile(xsdfilename,"utf-8",0);
	if (doc != NULL)
	{
		FILE * outfile = fopen(hfilename,"w");
		if (outfile == NULL)
		{
			printf("can't create %s - %s\n",hfilename,strerror(errno));
			exit(2);
		}
		fprintf(outfile,"/*\n automatic created by xsdclassgen\n*/\n\n");
		AddNamespace("xs","http://www.w3.org/2009/xml.xsd");
		AddType(new xsdType("xs:int",type_int,true));
		AddType(new xsdType("xs:string",type_string,true));
		AddType(new xsdType("xs:boolean",type_boolean,true));
		AddType(new xsdType("xs:decimal",type_decimal,true));
		AddType(new xsdType("xs:float",type_float,true));
		AddType(new xsdType("xs:double",type_double,true));
		AddType(new xsdType("xs:integer",type_integer,true));
		AddType(new xsdType("xs:long",type_long,true));
		AddType(new xsdType("xs:short",type_short,true));
		AddType(new xsdType("xs:byte",type_byte,true));
		AddType(new xsdType("xs:nonNegativeInteger",type_nonNegativeInteger,true));
		AddType(new xsdType("xs:unsignedLong",type_unsignedLong,true));
		AddType(new xsdType("xs:unsignedInt",type_unsignedInt,true));
		AddType(new xsdType("xs:unsignedShort",type_unsignedShort,true));
		AddType(new xsdType("xs:unsignedByte",type_unsignedByte,true));
		AddType(new xsdType("xs:unsignedPositiveInteger",type_positiveInteger,true));
		AddType(createDateTime());
		for_each_child(child,doc)
		{
			if (child->type == XML_ELEMENT_NODE)
			{
				xsd_keyword kw = Lookup(child->name);
				switch(kw)
				{
					case	xsd_schema:
						xmlschema = ParseSchema(child);
					break ;

					default:
					break ;
				}
			}
		}
		xsdTypeList deplist;
		for (NamespaceList::iterator nsi = namespaces.begin() ; nsi != namespaces.end() ; nsi++)
		{
			xsdNamespace * ns = *nsi ;
			ns->m_elements.CalcDependency(deplist);
		}
		for (typeIterator ti = deplist.begin() ; ti != deplist.end() ; ti++)
		{
			xsdType * type = *ti ;
			if (!type->isAnonym())
				type->GenCode(outfile,1,"",type->getCppName());
		}
		for (NamespaceList::iterator nsi = namespaces.begin() ; nsi != namespaces.end() ; nsi++)
		{
			xsdNamespace * ns = *nsi ;
			elementIterator ei ;
			fprintf(outfile,"/*\n Namespace %s\n*/\n\n",ns->m_name.c_str());
			for (ei = ns->m_elements.begin() ; ei != ns->m_elements.end(); ei++)
			{
				xsdElement * elem = *ei ;
				if (elem->m_type != NULL)
				{
					if (!elem->m_type->m_impl)
					{
						elem->m_type->GenCode(outfile,1,"",elem->getCppName());
					}
				}
			}
		}
	}
	return 0 ;
}
