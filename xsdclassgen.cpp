/*
 * xsdclassgen.cpp
 *
 *  Created on: 26.11.2012
 *      Author: dorwig
 */
#include <stdio.h>
#include <string.h>
#include <libxml2/libxml/parser.h>
#include "xsdclassgen.h"

enum xsd_keyword
{
	xsd_none,
	xsd_schema,
	xsd_element,
	xsd_simpleType,
	xsd_complexType,
	xsd_choice,
	xsd_sequence,
	xsd_restriction,
	xsd_base,
	xsd_enumeration,
	xsd_name,
	xsd_value,
	xsd_int,
	xsd_string,
	xsd_type,
	xsd_maxOccurs,
	xsd_minOccurs,
	xsd_list,
	xsd_union,
	xsd_minExclusive,
	xsd_minInclusive,
	xsd_maxExclusive,
	xsd_maxInclusive,
	xsd_totalDigits,
	xsd_fractionDigits,
	xsd_length,
	xsd_minLength,
	xsd_maxLength,
	xsd_whiteSpace,
	xsd_pattern,
	xsd_targeNamespace,
	xsd_xmlns,
};

struct xsd_keyword_entry
{
	xsd_keyword keyword;
	const char * name ;
};

#define ENTRY(x) {xsd_##x,#x}

struct xsd_keyword_entry keywordtable[] = {
		ENTRY(base),
    ENTRY(choice),
    ENTRY(complexType),
    ENTRY(element),
    ENTRY(enumeration),
    ENTRY(fractionDigits),
    ENTRY(int),
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
    ENTRY(sequence),
    ENTRY(simpleType),
    ENTRY(string),
  	ENTRY(targeNamespace),
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

xsdTypeList xsdType::m_alltypes;

std::list<xsdElement *> elementlist;

void xsdTypeList::Add(xsdType * type)
{
	xsdType * t = Find(type->getName());
	if (t != NULL)
	{
		if (t->m_tag == type_forward)
		{
			UpdateType(t,type);
			remove(t);
			push_back(type);
		}
	}
	else
	{
		push_back(type);
	}
}

xsdType * xsdTypeList::Find(const char * name)
{
	typeListIterator ti ;
	if (*name == 0)
		return NULL;
	for (ti = begin() ; ti != end() ; ti++)
	{
		xsdType * type = *ti ;
		if (!type->isAnonym() && type->m_name == name)
			return type ;
	}
	return NULL;
}

void AddElement(xsdElement * elem)
{
	printf("AddElement %s\n",elem->getName());
	elementlist.push_back(elem);
}

void UpdateType(std::list<xsdElement*> elemlist,xsdType * oldtype, xsdType * newtype)
{
	std::list<xsdElement*>::iterator it;
	for (it = elemlist.begin() ; it != elemlist.end() ; it++)
	{
		xsdElement * elem = *it ;
		if (elem->m_type == oldtype)
		{
			printf("Update type for Element %s to %s\n",elem->getName(),newtype->getName());
			elem->m_type = newtype ;
		}
	}
}

void UpdateType(xsdSequenceType * seq,xsdType * oldtype, xsdType * newtype)
{
	UpdateType(seq->m_list,oldtype,newtype);
}

void UpdateType(xsdComplexType * type,xsdType * oldtype, xsdType * newtype)
{
	if (type->m_sequence != NULL)
		UpdateType(type->m_sequence,oldtype,newtype);
}

void UpdateType(xsdType * oldtype,xsdType * newtype)
{
	std::list<xsdElement*>::iterator ei;
	for (ei = elementlist.begin() ; ei != elementlist.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		if (elem->m_type == oldtype)
			elem->m_type = newtype;
	}
	typeListIterator ti;
	for (ti = xsdType::m_alltypes.begin() ; ti != xsdType::m_alltypes.end() ; ti++)
	{
		xsdType * type = *ti;
		switch(type->m_tag)
		{
			case	type_complex:
				UpdateType((xsdComplexType*)type,oldtype,newtype);
			break ;

			default:
			break ;
		}
	}
}

xsdComplexType * ParseComplexType(xmlNodePtr type);
xsdSimpleType  * ParseSimpleType(xmlNodePtr type);

xsdElement * ParseElement(xmlNodePtr element)
{
	xmlNodePtr child = element->children;
	xmlAttrPtr attr  = element->properties;
	xsdElement * xsdelem     = NULL;
	xsdType    * xsdtype     = NULL;
	const char * xsdname     = NULL ;
	const char * xsdtypename = NULL;
	int minOccurs = 0 ;
	int maxOccurs = 0 ;
	while(attr != NULL)
	{
		xsd_keyword kw = Lookup(attr->name);
		child = attr->children;
		switch(kw)
		{
			case	xsd_name:
				xsdname = (const char*)child->content;
			break ;

			case	xsd_type:
			{
				const char * cp ;
				xsdtypename = (const char*)child->content;
				cp = strchr(xsdtypename,':');
				if (cp == NULL)
					cp = xsdtypename;
				else
					cp++;
				xsdtype = xsdType::m_alltypes.Find(cp);
				if (xsdtype == NULL)
				{
					xsdtype = new xsdType(cp,type_forward);
				}
			}
			break ;

			case	xsd_minOccurs:
				minOccurs = strtol((const char*)child->content,NULL,10);
			break ;

			case	xsd_maxOccurs:
				maxOccurs = strtol((const char*)child->content,NULL,10);
			break ;

			default:
				printf("attribute %s was not expected\n",attr->name);
			break ;
		}
		attr = attr->next;
	}
	child = element->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			xsd_keyword kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_element:
				break ;

				case	xsd_complexType:
					if (xsdtype != NULL)
					{
						printf("element %s already has a type\n",xsdname);
					}
					else
					{
						xsdtype = ParseComplexType(child);
					}
				break ;

				case	xsd_simpleType:
					xsdtype = ParseSimpleType(child);
				break ;

				default:
					printf("%s was not expected in <element> declaration\n",child->name);
				break ;
			}
		}
		child = child->next;
	}
	xsdelem = new xsdElement(xsdname,xsdtype);
	return xsdelem;
}

xsdSequenceType * ParseSequence(xmlNodePtr sequence)
{
	xsdSequenceType * xmlseq = new xsdSequenceType();
	xmlNodePtr child = sequence->children;
	xsd_keyword kw ;
	while(child != NULL)
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
					printf("%s was unexpected in Sequence\n",child->name);
				break ;
			}
		}
		child = child->next;
	}
	return xmlseq;
}

xsdChoiceType * ParseChoice(xmlNodePtr choice)
{
	xsdChoiceType * xmltype = NULL;
	return xmltype;
}

xsdComplexType * ParseComplexType(xmlNodePtr type)
{
	xmlNodePtr child = type->children;
	xmlAttrPtr attr  = type->properties;
	const char * xsdtypename = "";
	xsd_keyword kw ;
	while(attr != NULL)
	{
		kw = Lookup(attr->name);
		child = attr->children;
		switch(kw)
		{
			case	xsd_name:
				xsdtypename = (const char*)child->content;
			break ;

			default:
			break ;
		}
		attr = attr->next;
	}
	xsdComplexType * newtype = new xsdComplexType(xsdtypename,type_complex);
	child = type->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_sequence:
					newtype->m_sequence = ParseSequence(child);
					newtype->m_sequence->m_name = newtype->getName();
				break ;

				case	xsd_choice:
					newtype->m_choice = ParseChoice(child);
				break ;

				default:
				break ;
			}
		}
		child = child->next;
	}
	return newtype;
}

const char * ParseEnum(xmlNodePtr ptr)
{
	xsd_keyword kw ;
	xmlAttrPtr attr  = ptr->properties;
	while(attr != NULL)
	{
		kw = Lookup(attr->name);
		if (kw == xsd_value)
		{
			return (const char*)attr->children->content;
		}
		attr = attr->next;
	}
	return "";
}

int getIntAttr(xmlNodePtr ptr,const char * name)
{
	for (xmlAttrPtr attr = ptr->properties ; attr != NULL ; attr = attr->next)
	{
		if (strcmp((const char*)attr->name,name) == 0)
		{
			if (attr->children != NULL && attr->children->content != NULL)
			{
				return strtol((const char*)attr->children->content,NULL,10);
			}
		}
	}
	return 0 ;
}

xsdRestriction * ParseRestriction(xmlNodePtr rest)
{
	xmlNodePtr child = rest->children;
	xmlAttrPtr attr  = rest->properties;
	xsdRestriction * xmlrest = new xsdRestriction();
	xsd_keyword kw ;
	while(attr != NULL)
	{
		kw = Lookup(attr->name);
		child = attr->children;
		switch(kw)
		{
			case	xsd_base:
				xmlrest->m_base = (const char*)child->content;
			break ;
			default:
			break ;
		}
		attr = attr->next;
	}
	child = rest->children;
	while(child != NULL)
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
		child = child->next ;
	}
	return xmlrest;
}

xsdSimpleType * ParseSimpleType(xmlNodePtr type)
{
	xsdSimpleType * xsdtype = NULL;
	const char * xsdtypename = "" ;
	xmlNodePtr child = type->children;
	xmlAttrPtr attr  = type->properties;
	xsd_keyword kw ;
	while(attr != NULL)
	{
		kw = Lookup(attr->name);
		child = attr->children;
		switch(kw)
		{
			case	xsd_name:
				xsdtypename = (const char*)child->content;
			break ;
			default:
			break ;
		}
		attr = attr->next;
	}
	xsdtype = new xsdSimpleType(xsdtypename);
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xsdtype->m_rest = ParseRestriction(child);
				break ;

				default:
				break;
			}
		}
		child = child->next;
	}
	return xsdtype;
}

xsdSchema * ParseSchema(xmlNodePtr schema)
{
	xsdSchema * xmlschema = new xsdSchema();
	xmlAttrPtr attr = schema->properties;
	xmlNodePtr child ;
	xmlNsPtr xmlns ;
	xsd_keyword kw;

	while(attr != NULL)
	{
		kw = Lookup(attr->name);
		printf("Schema attr %s %s\n",attr->name,attr->children->content);
		switch(kw)
		{
			case	xsd_targeNamespace:
			break ;
			case	xsd_xmlns:
			break ;
			default:
			break ;
		}
		attr = attr->next;
	}
	xmlns = schema->nsDef;
	child = schema->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_element:
					AddElement(ParseElement(child));
				break ;

				case	xsd_complexType:
					ParseComplexType(child);
				break ;

				case	xsd_simpleType:
					ParseSimpleType(child);
				break ;

				default:
				break ;
			}
		}
		child = child->next;
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

void xsdType::GenCode(FILE * out,int indent,const char * elemname)
{
	const char * name ;
	switch(m_tag)
	{
		case  type_float:					name = "float";					break ;
		case	type_double:				name = "double";				break ;
		case	type_byte:    			name = "char";					break ;
		case	type_unsignedByte: 	name = "unsigned char";	break ;
		case	type_int:						name = "int";						break ;
		case	type_long:    			name = "long";					break ;
		case	type_unsignedInt: 	name = "unsigned int";	break ;
		case	type_boolean:				name = "bool";					break ;
		case	type_short:					name = "short";					break ;
		case  type_unsignedShort:	name = "unsigned short";break ;
		default:               		name = getName();    		break ;
	}
	if (elemname)
	{
		Indent(out,indent);
		fprintf(out,"%s %s;\n",name,elemname);
	}
}

void xsdRestriction::GenCode(FILE * out,int indent,const char * elemname)
{
	const char * n = "int";
	Indent(out,indent);
	if (!m_base.empty())
		n = m_base.c_str();
	fprintf(out,"%s %s;\n",n,elemname);
}

void xsdSimpleType::GenCode(FILE * out,int indent,const char * elemname)
{
	if (m_rest != NULL)
		m_rest->GenCode(out,indent,elemname);
	else
		xsdType::GenCode(out,indent,elemname);
}

void xsdSequenceType::GenCode(FILE * out,int indent,const char * elemname)
{
	std::list<xsdElement*>::iterator ei;
	Indent(out,indent);
	fprintf(out,"struct %s\n",getName());
	Indent(out,indent);
	fprintf(out,"{\n");
	indent++;
	for (ei = m_list.begin() ; ei != m_list.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenCode(out,indent);
	}
	indent--;
	Indent(out,indent);
	fprintf(out,"} %s;\n\n",elemname == NULL ? "" : elemname);
}

void xsdComplexType::GenCode(FILE * out,int indent,const char * elemname)
{
	if (m_sequence != NULL)
	{
		m_sequence->GenCode(out,indent,elemname);
	}
}

void xsdElement::GenCode(FILE *out,int indent)
{
	m_type->GenCode(out,indent,getName());
}

int main(int argc, char * argv[])
{
	xsdSchema * xmlschema = NULL;
	xmlDocPtr doc = xmlReadFile(argv[1],"utf-8",0);
	if (doc != NULL)
	{
		new xsdType("int",type_int);
		new xsdType("string",type_string);
		new xsdType("boolean",type_boolean);
		new xsdType("decimal",type_decimal);
		new xsdType("float",type_float);
		new xsdType("double",type_double);
		xmlNodePtr child;
		child = doc->xmlChildrenNode ;
		if (child != NULL)
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
		typeListIterator ti ;
		int indent = 0 ;
		for (ti = xsdType::m_alltypes.begin() ; ti != xsdType::m_alltypes.end() ; ti++)
		{
			(*ti)->GenCode(stdout,indent,NULL);
		}
	}
}



