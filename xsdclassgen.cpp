/*
 * xsdclassgen.cpp
 *
 *  Created on: 26.11.2012
 *      Author: dorwig
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
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

int xsdType::m_count;
xsdTypeList m_alltypes;
xsdElementList elementlist;

#ifdef UPDATE_TYPE
void UpdateType(xsdType * oldtype,xsdType * newtype);
#endif

void xsdTypeList::Add(xsdType * type)
{
	xsdType * t = Find(type->getName());
	if (t != NULL)
	{
		if (t->m_tag == type_forward)
		{
#ifdef UPDATE_TYPE
			UpdateType(t,type);
			remove(t);
			delete t ;
			push_back(type);
#endif
		}
		else
		{
			printf("%s is already declared\n",type->getName());
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

#ifdef UPDATE_TYPE
void UpdateType(xsdComplexType * type,xsdType * oldtype, xsdType * newtype);

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
		else
		{
			switch(elem->m_type->m_tag)
			{
				case	type_complex:
					UpdateType((xsdComplexType*)elem->m_type,oldtype,newtype);
				break ;

				default:
				break ;
			}
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
	UpdateType(elementlist,oldtype,newtype);
	typeListIterator ti;
	for (ti = m_alltypes.begin() ; ti != m_alltypes.end() ; ti++)
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
#endif

xsdComplexType * ParseComplexType(xmlNodePtr type);
xsdSimpleType  * ParseSimpleType(xmlNodePtr type);

xsdElement * ParseElement(xmlNodePtr element)
{
	xmlNodePtr child = element->children;
	xmlAttrPtr attr  = element->properties;
	xsdElement * xsdelem     = NULL;
	xsdType    * xsdtype     = NULL;
	const char * xsdname     = NULL ;
	const char * xsdtypename = "";
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
				if (cp != NULL)
					xsdtypename = ++cp ;
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
					if (*xsdtypename != 0)
					{
						printf("element %s already has a type %s\n",xsdname,xsdtypename);
					}
					else
					{
						xsdtype = ParseComplexType(child);
					}
				break ;

				case	xsd_simpleType:
					if (*xsdtypename != 0)
						printf("element %s already has a type %s\n",xsdname,xsdtypename);
					else
						xsdtype = ParseSimpleType(child);
				break ;

				default:
					printf("%s was not expected in <element> declaration\n",child->name);
				break ;
			}
		}
		child = child->next;
	}
	xsdelem = new xsdElement(xsdname,xsdtypename,xsdtype);
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
	child = type->children;
	while(child != NULL)
	{
		if (child->type == XML_ELEMENT_NODE)
		{
			kw = Lookup(child->name);
			switch(kw)
			{
				case	xsd_restriction:
					xsdtype->m_rest = ParseRestriction(child);
#if 0
					if (*xsdtypename == 0 && !xsdtype->m_rest->m_enumvalues.empty())
					{
						char enumname[10];
						sprintf(enumname,"e_an%04d",++nextenum);
						xsdtype->m_name = enumname ;
					}
#endif
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
					m_alltypes.Add(ParseComplexType(child));
				break ;

				case	xsd_simpleType:
					m_alltypes.Add(ParseSimpleType(child));
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
	const char * name = getCppName();
	if (elemname)
	{
		Indent(out,indent);
		fprintf(out,"%s %s;\n",name,elemname);
	}
}

const char * xsdType::getCppName()
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
		case  type_string:        name = "std::string";   break ;
		default:               		name = getName();    		break ;
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
		std::list<std::string>::iterator si ;
		for (si = m_enumvalues.begin() ; si != m_enumvalues.end() ; si++)
		{
			Indent(out,indent+1);
			fprintf(out,"%s,\n",(*si).c_str());
		}
		Indent(out,indent);
		fprintf(out,"} %s ;\n",elemname);
	}
	else
	{
		Indent(out,indent);
		fprintf(out,"%s %s;\n",m_base.c_str(),elemname);
	}
}

void xsdSimpleType::GenCode(FILE * out,int indent,const char * elemname)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_rest != NULL)
			m_rest->GenCode(out,indent,elemname,getName());
		else
			xsdType::GenCode(out,indent,elemname);
	}
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
	if (!m_impl)
	{
		m_impl = true;
		if (m_sequence != NULL)
		{
			m_sequence->GenCode(out,indent,elemname);
		}
	}
}

void xsdElement::GenCode(FILE *out,int indent)
{
	int stop ;
	if (m_name == "Update")
	{
		stop = 1 ;
	}
	if (m_type != NULL)
		m_type->GenCode(out,indent,getName());
	else
	{
		xsdType * type = m_alltypes.Find(m_typename.c_str());
		Indent(out,indent);
		if (type == NULL)
		{
			printf("Element %s has unknown type %s\n",getName(),m_typename.c_str());
			fprintf(out,"%s %s;\n",m_typename.c_str(),getName());
		}
		else
		{
			fprintf(out,"%s %s;\n",type->getCppName(),getName());
		}
	}
}

void CalcDependency(xsdElementList & elist, xsdTypeList & list);

void CalcDependen(xsdSequenceType * seq,xsdTypeList & list)
{
	CalcDependency(seq->m_list,list);
}

void CalcDependeny(xsdComplexType * type,xsdTypeList & list)
{
	if (type->m_sequence != NULL)
	{
		CalcDependen(type->m_sequence,list);
	}
}

void CalcDependency(xsdSimpleType* type,xsdTypeList & list)
{
	if (type->m_name.empty())
		return ;
	if (type->m_rest != NULL && !type->m_rest->m_enumvalues.empty())
	{
		if (!type->m_indeplist)
		{
			printf("add dependency %s\n",type->getName());
			type->m_indeplist = true ;
			list.push_back(type);
		}
	}
}

void CalcDependency(xsdElementList & elist, xsdTypeList & list)
{
	elementIterator eli ;
	typeListIterator tli ;
	for (eli = elist.begin() ; eli != elist.end(); eli++)
	{
		xsdElement * elem = *eli ;
		xsdType * type = elem->m_type;
		if (elem->m_name == "Update")
			printf("CalcDependency for %s\n",elem->getName());
		if (type == NULL)
		{
			if (!elem->m_typename.empty())
			{
				type = m_alltypes.Find(elem->m_typename.c_str());
				if (type == NULL)
				{
					printf("element %s: type %s not found\n",elem->getName(),elem->m_typename.c_str());
				}
			}
			else
			{
				printf("element %s has not type\n",elem->getName());
			}
		}
		if (type != NULL)
		{
			switch(type->m_tag)
			{
				case	type_complex:
					if (!type->m_indeplist)
					{
						type->m_indeplist = true ;
						CalcDependeny((xsdComplexType*)type,list);
						printf("add dependency %s\n",type->getName());
						list.push_back(type);
					}
				break ;

				case	type_simple:
					CalcDependency((xsdSimpleType*)type,list);
				break ;

				default:
				break ;
			}
		}
	}
}

int main(int argc, char * argv[])
{
	xsdSchema * xmlschema = NULL;
	const char * xsdfilename = NULL;
	char hfilename[256];

	for (int i = 1 ; i < argc ; i++)
	{
		xsdfilename = argv[i];
		break ;
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
		m_alltypes.Add(new xsdType("int",type_int,true));
		m_alltypes.Add(new xsdType("string",type_string,true));
		m_alltypes.Add(new xsdType("boolean",type_boolean,true));
		m_alltypes.Add(new xsdType("decimal",type_decimal,true));
		m_alltypes.Add(new xsdType("float",type_float,true));
		m_alltypes.Add(new xsdType("double",type_double,true));
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
		xsdTypeList deplist;
		CalcDependency(elementlist,deplist);

		typeListIterator ti ;
		for (ti = deplist.begin() ; ti != deplist.end() ; ti++)
		{
			xsdType * type = *ti ;
			if (!type->isAnonym())
				type->GenCode(outfile,1,"");
		}
	}
	return 0 ;
}



