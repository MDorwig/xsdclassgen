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
  xsd_fractionDigits,
  xsd_group,
  xsd_import,
  xsd_int,
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
    ENTRY(fractionDigits),
    ENTRY(group),
    ENTRY(import),
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
  	ENTRY(schemaLocation),
    ENTRY(sequence),
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

void AddNamespace(const char * prefix,const char * href)
{
	xsdNamespace * ns = FindNamespace(prefix);
	if (ns == NULL)
	{
		ns = new xsdNamespace(prefix,href);
		namespaces.push_back(ns);
	}
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

#ifdef UPDATE_TYPE
void UpdateType(xsdType * oldtype,xsdType * newtype);
#endif

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

xsdAttribute   * ParseAttribute(xmlNodePtr node)
{
	xsd_keyword kw ;
	xsdAttribute * xsdattr = new xsdAttribute();
	for_each_attr(attr,node)
	{
		kw = Lookup(attr->name);
		switch(kw)
		{
			case	xsd_name:
				xsdattr->m_name = getContent(attr->children);
			break ;

			case	xsd_default:
				xsdattr->m_name = getContent(attr->children);
			default:
			case	xsd_type:
				xsdattr->m_type = FindType(getContent(attr->children));
			break ;
			break ;
		}
	}
	return xsdattr;
}

xsdElement * ParseElement(xmlNodePtr element)
{
	xsdElement * xsdelem     = NULL;
	xsdType    * xsdtype     = NULL;
	const char * xsdname     = NULL;
	const char * xsdtypename = NULL;
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
			{
				xsdtypename = getContent(attr->children);
				xsdtype = FindType(xsdtypename);
			}
			break ;

			case	xsd_minOccurs:
				minOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			case	xsd_maxOccurs:
				maxOccurs = strtol(getContent(attr->children),NULL,10);
			break ;

			default:
				printf("attribute %s was not expected\n",attr->name);
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
				case	xsd_element:
				break ;

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
					printf("%s was not expected in <element> declaration\n",child->name);
				break ;
			}
		}
	}
	xsdelem = new xsdElement(xsdname,xsdtypename,xsdtype);
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
					printf("%s was unexpected in %s\n",child->name,sequence->name);
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
					printf("%s was unexpected in %s\n",child->name,all->name);
				break ;
			}
		}
	}
	return xsdtype;
}

xsdChoice * ParseChoice(xmlNodePtr choice)
{
	xsd_keyword kw ;
	for_each_attr(attr,choice)
	{
		kw = Lookup(attr->name);
	}
	xsdChoice * xsdchoice = new xsdChoice();
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
				case	xsd_any:
				default:
					printf("element %s not implemented in <choice>\n",child->name);
				break ;
			}
		}
	}

	return xsdchoice;
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

				default:
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
				xmlrest->m_base = getContent(attr->children);
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
				const char * value = getContent(attr->children);
				const char * cp = strrchr(value,'/');
				if (cp != NULL)
					cp++;
				else
					cp = value;
				strcpy(filename,cp);
				FILE * f = fopen(filename,"r");
				if (f == NULL)
				{
					printf("can't open import \"%s\"\n",filename);
					printf("download \"%s\" and install locally\n",getContent(attr->children));
				}
				else
				{
					fclose(f);
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
					ParseImport(child);
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
					printf("element %s not implemented in %s\n",child->name,schema->name);
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
			fprintf(out,"%s,\n",(*si)->getCppName());
		}
		Indent(out,indent);
		fprintf(out,"} %s ;\n",elemname);
	}
	else
	{
		Indent(out,indent);
		if (*elemname == 0)
			fprintf(out,"typedef %s %s;\n",m_base.c_str(),simplename);
		else
			fprintf(out,"%s %s;\n",m_base.c_str(),elemname);
	}
}

void xsdSimpleType::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	if (!m_impl)
	{
		m_impl = true;
		if (m_rest != NULL)
			m_rest->GenCode(out,indent,elemname,getCppName());
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
	for (ei = m_list.begin() ; ei != m_list.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenCode(out,indent);
	}
	indent--;
	Indent(out,indent);
	fprintf(out,"} %s;\n\n",elemname == NULL ? "" : elemname);
}

void xsdAll::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	std::list<xsdElement*>::iterator ei;
	Indent(out,indent);
	fprintf(out,"struct %s\n",supertypename);
	Indent(out,indent);
	fprintf(out,"{\n");
	indent++;
	for (ei = m_elements.begin() ; ei != m_elements.end() ; ei++)
	{
		xsdElement * elem = *ei ;
		elem->GenCode(out,indent);
	}
	indent--;
	Indent(out,indent);
	fprintf(out,"} %s;\n\n",elemname == NULL ? "" : elemname);
}

void xsdChoice::GenCode(FILE *out,int indent,const char * elemname,const char * supertypename)
{
	if (!m_impl)
	{
		m_impl = true ;
		for (elementIterator ei = m_elements.begin(); ei != m_elements.end() ; ei++)
		{
			xsdElement * elem = *ei;
			elem->GenCode(out,indent);
		}
	}
}

void xsdComplexType::GenCode(FILE * out,int indent,const char * elemname,const char * supertypename)
{
	if (!m_impl)
	{
		if (!m_cname.empty())
			supertypename = getCppName();
		m_impl = true;
		if (m_type != NULL)
		{
			switch(m_type->m_tag)
			{
				case	type_sequence:
					((xsdSequence*)m_type)->GenCode(out,indent,elemname,supertypename);
				break;

				case	type_choice:
					((xsdChoice*)m_type)->GenCode(out,indent,elemname,supertypename);
				break ;

				case	type_all:
					((xsdAll*)m_type)->GenCode(out,indent,elemname,supertypename);
				break ;

				default:
				break ;
			}
		}
	}
}

void xsdElement::GenCode(FILE *out,int indent)
{
	if (m_type != NULL)
		m_type->GenCode(out,indent,getName(),"");
	else
	{
		xsdType * type = FindType(m_typename.c_str());
		Indent(out,indent);
		if (type == NULL)
		{
			printf("Element %s has unknown type %s\n",getName(),m_typename.c_str());
			fprintf(out,"%s %s;\n",m_typename.c_str(),getName());
		}
		else
		{
			fprintf(out,"%s %s;\n",type->getCppName(),getCppName());
		}
	}
}

void CalcDependency(xsdElementList & elist, xsdTypeList & list);

void CalcDependency(xsdSequence * seq,xsdTypeList & list)
{
	CalcDependency(seq->m_list,list);
}

void CalcDependency(xsdAll * all,xsdTypeList & list)
{
	CalcDependency(all->m_elements,list);
}

void CalcDependency(xsdChoice * type,xsdTypeList & list)
{
	CalcDependency(type->m_elements,list);
	for (sequenceIterator si = type->m_sequences.begin() ; si != type->m_sequences.end() ; si++)
	{
		CalcDependency(*si,list);
	}
}
void CalcDependeny(xsdComplexType * type,xsdTypeList & list)
{
	if (type->m_type != NULL)
	{
		switch(type->m_type->m_tag)
		{
			case	type_sequence:
				CalcDependency((xsdSequence*)type->m_type,list);
			break ;
			case	type_choice:
				CalcDependency((xsdChoice*)type->m_type,list);
			break ;
			case	type_all:
				CalcDependency((xsdAll*)type->m_type,list);
			break ;
			default:
			break ;
		}
	}
}

void CalcDependency(xsdSimpleType* type,xsdTypeList & list)
{
	if (type->m_name.empty())
		return ;
	if (type->m_rest != NULL && !type->m_indeplist)
	{
		printf("add dependency %s\n",type->getName());
		type->m_indeplist = true ;
		list.push_back(type);
	}
}

void CalcDependency(xsdElementList & elist, xsdTypeList & list)
{
	elementIterator eli ;
	typeIterator tli ;
	for (eli = elist.begin() ; eli != elist.end(); eli++)
	{
		xsdElement * elem = *eli ;
		xsdType * type = elem->m_type;
		printf("CalcDependency for %s\n",elem->getName());
		if (type == NULL)
		{
			if (!elem->m_typename.empty())
			{
				type = FindType(elem->m_typename.c_str());
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

#if 0
		CalcDependency(elementlist,deplist);

		typeListIterator ti ;
		for (ti = deplist.begin() ; ti != deplist.end() ; ti++)
		{
			xsdType * type = *ti ;
			if (!type->isAnonym())
				type->GenCode(outfile,1,"",type->getCppName());
		}
		elementIterator ei ;
		for (ei = elementlist.begin() ; ei != elementlist.end(); ei++)
		{
			xsdElement * elem = *ei ;
			if (elem->m_type != NULL)
			{
				if (!elem->m_type->m_impl)
				{
					if (elem->m_type->m_name.empty())
						elem->m_type->m_name = elem->m_name;
					elem->m_type->GenCode(outfile,1,"","");
				}
			}
		}
#endif
	}
	return 0 ;
}



