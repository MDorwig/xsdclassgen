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
    ENTRY(totalDigits),
    ENTRY(type),
    ENTRY(union),
    ENTRY(value),
    ENTRY(whiteSpace),
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

CxmlElement * ParseElement(xmlNodePtr element)
{
	xmlNodePtr child = element->children;
	xmlAttrPtr attr  = element->properties;
	CxmlElement * xmlelem = NULL;
	CxmlType    * xmltype = NULL;
	const char * etypename=NULL;
	int minOccurs = 0 ;
	int maxOccurs = 0 ;
	while(attr != NULL)
	{
		xsd_keyword kw = Lookup(attr->name);
		child = attr->children;
		switch(kw)
		{
			case	xsd_name:
				xmlelem = new CxmlElement(child->content,NULL,BAD_CAST "");
			break ;
			case	xsd_type:
				etypename = (const char*)child->content;
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
	if (xmlelem != NULL)
	{
		if (etypename != NULL)
			xmlelem->m_typename = etypename;
		xmlelem->m_type = xmltype;
		xmlelem->m_maxOccurs = maxOccurs;
		xmlelem->m_minOccurs = minOccurs;
	}
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			xsd_keyword kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_element:
				break ;

				default:
				break ;
			}
		}
		child = child->next;
	}
	return xmlelem;
}

CxmlSequenceType * ParseSequence(xmlNodePtr sequence)
{
	CxmlSequenceType * xmlseq = new CxmlSequenceType();
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

CxmlChoiceType * ParseChoice(xmlNodePtr choice)
{
	CxmlChoiceType * xmltype = NULL;
	return xmltype;
}

CxmlComplexType * ParseComplexType(xmlNodePtr type)
{
	xmlNodePtr child = type->children;
	xmlAttrPtr attr  = type->properties;
	CxmlComplexType * xmltype = NULL;
	xsd_keyword kw ;
	while(attr != NULL)
	{
		kw = Lookup(attr->name);
		child = attr->children;
		switch(kw)
		{
			case	xsd_name:
				xmltype = new CxmlComplexType(child->content,type_complex);
			break ;
			default:
			break ;
		}
		attr = attr->next;
	}
	child = type->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_sequence:
					xmltype->m_sequence = ParseSequence(child);
				break ;

				case	xsd_choice:
					xmltype->m_choice = ParseChoice(child);
				break ;

				default:
				break ;
			}
		}
		child = child->next;
	}
	return xmltype;
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

CxmlRestriction * ParseRestriction(xmlNodePtr rest)
{
	xmlNodePtr child = rest->children;
	xmlAttrPtr attr  = rest->properties;
	CxmlRestriction * xmlrest = new CxmlRestriction();
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
				break ;

				case	xsd_maxExclusive:
				break ;

				case	xsd_minInclusive:
				break ;

				case	xsd_maxInclusive:
				break ;

				case	xsd_totalDigits:
				break ;
				case	xsd_fractionDigits:
				break ;
				case xsd_length:
				break ;
				case xsd_minLength:
				break ;
				case xsd_maxLength:
				break ;
				default:
				break ;
			}
		}
		child = child->next ;
	}
	return xmlrest;
}

CxmlSimpleType * ParseSimpleType(xmlNodePtr type)
{
	CxmlSimpleType * xmltype = NULL;
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
				xmltype = new CxmlSimpleType(child->content);
			break ;
			default:
			break ;
		}
		attr = attr->next;
	}
	child = type->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xmltype->m_rest = ParseRestriction(child);
				break ;

				default:
				break;
			}
		}
		child = child->next;
	}
	return xmltype;
}

CxmlSchema * ParseSchema(xmlNodePtr schema)
{
	CxmlSchema * xmlschema = new CxmlSchema();
	xmlNodePtr child = schema->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			xsd_keyword kw = Lookup(child->name) ;
			switch(kw)
			{
				case	xsd_element:
					xmlschema->addElement(ParseElement(child));
				break ;

				case	xsd_complexType:
					xmlschema->addType(ParseComplexType(child));
				break ;

				case	xsd_simpleType:
					xmlschema->addType(ParseSimpleType(child));
				break ;

				default:
				break ;
			}
		}
		child = child->next;
	}
	return xmlschema;
}

int main(int argc, char * argv[])
{
	CxmlSchema * xmlschema = NULL;
	xmlDocPtr doc = xmlReadFile(argv[1],"utf-8",0);
	if (doc != NULL)
	{
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
		std::list<CxmlType*>::iterator ti ;
		for (ti = xmlschema->m_types.begin() ; ti != xmlschema->m_types.end() ; ti++)
		{
			switch((*ti)->m_tag)
			{
				case	type_int:
				break ;
				case	type_string:
				break ;
				case	type_typename:
				break ;
				case	type_complex:
				{
					CxmlComplexType * c = (CxmlComplexType*)*ti ;
					printf("struct %s {\n",c->m_name.c_str());
					if (c->m_sequence != NULL)
					{
						CxmlSequenceType * seq = c->m_sequence;
						std::list<CxmlElement*>::iterator ei;
						for (ei = seq->m_list.begin() ; ei != seq->m_list.end() ; ei++)
						{
							CxmlElement * elem = *ei ;
							printf("\t%s %s;\n",elem->m_name.c_str(),elem->m_typename.c_str());
						}
					}
					printf("};\n");
				}
				break ;
				case	type_simple:
				break ;
				case	type_restriction:
				break ;
				case	type_sequence:
				break ;
			}
		}
	}
}



