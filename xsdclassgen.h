
#ifndef XSDCLASSGEN_H_INCLUDED
#define XSDCLASSGEN_H_INCLUDED

#include <list>
#include <string.h>
#include <string>

class xsdElement;
class xsdType ;
class xsdChoice;
class xsdGroup;
class xsdSequence;
class xsdAttribute;
class xsdTypeList;
class xsdAttrList;
class xsdGroupList;
class xsdSimpleType;

#include "symtab.h"
#include "cppfile.h"

typedef std::list<xsdType*>::iterator typeIterator;

typedef std::list<xsdChoice*> xsdChoiceList ;
typedef xsdChoiceList::iterator choiceIterator ;

std::string MakeIdentifier(const char * prefix,const char * name);

class xsdTypename // für vorwärtsdeclarationen
{
public:
	xsdTypename(const char * name)
	{
		if (strchr(name,':') != NULL)
		{
			while(*name != ':')
				m_ns += *name++;
			name++;
		}
		m_name       = name;
		m_cname      = MakeIdentifier("",name);
	}

	const char * getName()
	{
		return m_name.c_str();
	}

	const char * getCppName()
	{
		return m_cname.c_str();
	}

	std::string m_ns ;
	std::string m_name ;
	std::string m_cname;
};

class xsdElement
{
public:
	xsdElement(const char * name,const char * id,xsdTypename * typname,xsdType * type,int minOccurs,int maxOccurs)
	{
		if (strchr(name,':') != NULL)
		{
			while(*name != ':')
				m_ns += *name++;
			name++;
		}
		m_name       = name;
		if (id)
			m_cname = MakeIdentifier("m_",id) ;
		else
			m_cname      = MakeIdentifier("m_",name);
		m_typename   = typname;
		m_type       = type ;
		m_minOccurs  = minOccurs;
		m_maxOccurs  = maxOccurs;
	}

	~xsdElement()
	{
	}

	const char * getName()
	{
		return m_name.c_str();
	}

	const char * getCppName()
	{
		return m_cname.c_str();
	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);
	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	xsdTypename * m_typename;
	xsdType *   m_type ;
	int         m_minOccurs;
	int         m_maxOccurs;
};


class xsdEnumValue
{
public:
	xsdEnumValue(const char * value)
	{
		m_Value = value;
		m_cname = MakeIdentifier("e",value);
	}
	const char * getName()
	{
		return m_Value.c_str();
	}

	const char * getCppName()
	{
		return m_cname.c_str();
	}

	std::string m_Value ;
	std::string m_cname;
};

typedef std::list<xsdEnumValue *> xsdEnumValueList;
typedef xsdEnumValueList::iterator enumIterator;

enum typetag
{
	type_forward, // forward reference to an unknown type
	type_string,
	type_boolean,
	type_decimal,
	type_float,
	type_double,
	type_duration,
	type_dateTime,
	type_extension,
	type_simpleContent,
	type_time,
	type_date,
	type_gYearMonth,
	type_gYear,
	type_gMonthDay,
	type_gDay,
	type_gMonth,
	type_hexBinary,
	type_base64Binary,
	type_anyURI,
	type_QName,
	type_NOTATION,
	type_normalizedString,
	type_token,
	type_language,
	type_NMTOKEN,
	type_NMTOKENS,
	type_Name,
	type_NCName,
	type_ID,
	type_IDREF,
	type_IDREFS,
	type_ENTITY,
	type_ENTITIES,
	type_integer,
	type_nonPositiveInteger,
	type_negativeInteger,
	type_long,
	type_int,
	type_short,
	type_byte,
	type_enum,
	type_nonNegativeInteger,
	type_unsignedLong,
	type_unsignedInt,
	type_unsignedShort,
	type_unsignedByte,
	type_positiveInteger,
	type_yearMonthDuration,
	type_dayTimeDuration,
	type_dateTimeStamp,
	type_complex,
	type_simple,
	type_restriction,
	type_sequence,
	type_list,
	type_union,
	type_group,
	type_choice,
	type_all,
};

class xsdTypeList : public std::list<xsdType*>
{
public:
	xsdTypeList()
	{
	}

	xsdType * Find(const char * name);
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,bool choice);
};

class xsdElementList : public std::list<xsdElement*>
{
public:
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
};

typedef xsdElementList::iterator elementIterator ;


class xsdAttribute
{
public:
	enum eUse
	{
		eUse_Optional,
		eUse_Prohibited,
		eUse_Required,
	};

	xsdAttribute(const char * name,const char * defval,const char * fixed,eUse use,xsdType * type)
	{
		m_name    = name ;
		m_cname   = MakeIdentifier("a_",name);
		m_type    = type;
		m_default = defval;
		m_fixed   = fixed;
	}
	std::string m_name ;
	std::string m_cname;
	xsdType * m_type ;
	std::string m_default;
	std::string m_fixed;
	eUse        m_use ;
	const char * getName() { return m_name.c_str();}
	const char * getCppName() { return m_cname.c_str();}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
};

class xsdAttrList : public std::list<xsdAttribute*>
{
public:
	void GenHeader(CppFile & out,int indent);
};

typedef xsdAttrList::iterator attrIterator ;


class xsdType
{
public:
	xsdType(const char * name,typetag tag,xsdType * parent,bool builtin = false)
	{
		if (name == NULL)
			name = "";
		if (name[0] == 0)
			m_local = true ;
		else
		{
			if (strchr(name,':') != NULL)
			{
				while(*name != ':')
					m_ns += *name++;
				name++;
			}
			m_name = name;
			m_cname = MakeIdentifier(NULL,name);
		}
		m_parent=parent;
		m_tag = tag ;
		m_id = ++m_count;
		m_impl = builtin ;
		m_indeplist = false ;
	}

	virtual ~xsdType()
	{
	}

	virtual void CalcDependency(xsdTypeList & list)
	{
	}

	const char * getName()
	{
		return m_name.c_str();
	}

	const char * getCppName() ;

	/*
	 * erzeugt einen namen für struct <name> oder enum <name>
	 * entweder aus dem supertypename oder <prefix><elemname>
	 */
	std::string MakeTag(const char * prefix,const char * elemname,const char * supertypeename)
	{
		std::string s = prefix ;
		if (*supertypeename == 0)
		{
			if (strncmp(elemname,"m_",2) == 0)
				elemname+=2 ;
			s += elemname;
		}
		else
		  s += supertypeename;
		return s ;
	}

	bool isLocal()
	{
		return m_local ;
	}
	virtual void GenHeader(CppFile & out,int indent,const char * elemname);
	virtual void GenHeader(CppFile & out,int indent);
	virtual void GenImpl(CppFile & out,Symtab & st);
	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	xsdType *   m_parent;
	bool        m_local;
	typetag 		m_tag;
	int     		m_id ;
	bool    		m_impl;
	bool    		m_indeplist;
	static int 	m_count;
};

class xsdEnum : public xsdType
{
public:
	xsdEnum(xsdType * parent) : xsdType("",type_enum,parent)
	{
		const char * simplename = parent->getCppName();
		m_structname = simplename;
		m_enumname   = "e_" ;
		m_enumname  += simplename;
	}

	void AddValue(xsdEnumValue * val)
	{
		m_values.push_back(val);
	}

	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);
	xsdEnumValueList m_values;
	std::string m_enumname;
	std::string m_structname;
};


class xsdList : public xsdType
{
public:
	xsdList(const char * itemTypename,xsdType * type,xsdType * parent) : xsdType("",type_list,parent)
	{
		m_typename = NULL;
		if (itemTypename != NULL)
			m_typename = new xsdTypename(itemTypename);
		m_type = type ;
	}
	void CalcDependency(xsdTypeList & list);
	xsdTypename * m_typename ;
	xsdType     * m_type ;

	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);


};

class xsdUnion: public xsdType
{
public:
	xsdUnion(const char * memberTypes,xsdType * parent) : xsdType("",type_union,parent)
	{

	}
	std::list<xsdTypename*> m_memberTypes;
	xsdTypeList m_types ;
};

class xsdRestriction: public xsdType
{
public:
	xsdRestriction(xsdType * parent) : xsdType("",type_restriction,parent)
	{
		m_minExclusive = 0 ;
		m_minInclusive = 0 ;
		m_maxExclusive = 0 ;
		m_maxInclusive = 0 ;
		m_totalDigits = 0 ;
		m_fractionDigits = 0 ;
		m_length = 0 ;
		m_minLength = 0 ;
		m_maxLength = 1 ;
		m_whiteSpace = 0 ;
		m_base = NULL;
		m_simple = NULL;
		m_enum = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);


	xsdType       * m_base ;
	xsdEnum       * m_enum;
	xsdSimpleType * m_simple;
	int m_minExclusive;
	int m_minInclusive;
	int m_maxExclusive;
	int m_maxInclusive;
	int m_totalDigits;
	int m_fractionDigits;
	int m_length;
	int m_minLength;
	int m_maxLength;
	int m_whiteSpace;
};

class xsdExtension : public xsdType
{
public:
	xsdExtension(xsdTypename * basetypename,xsdType * parent) : xsdType("",type_extension,parent)
	{
		m_basetypename = basetypename;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
//	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);


	xsdTypename * m_basetypename ;
	xsdAttrList   m_attributes;
};

class xsdSimpleContent : public xsdType
{
public:
	xsdSimpleContent(xsdType * parent) : xsdType("",type_simpleContent,parent)
	{
		m_content = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);

	xsdType * m_content; // xsd_Restriction oder xsd_Extension
};

class xsdSimpleType : public xsdType
{
public:
	xsdSimpleType(const char * name,const char * elemname,xsdType * parent) : xsdType(name,type_simple,parent)
	{
		if (m_name.empty() && *elemname != 0)
		{
			/*
			 * simpleType ohne namen aber mit zugehörigem element
			 * es wird ein typname in der form "<elemname>_t" erzeugt
			 */
			m_name = elemname;
			m_name += "_t";
			m_cname = MakeIdentifier("",m_name.c_str());
			m_local= true;
		}
		m_rest = NULL;
		m_list = NULL;
		m_union= NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);
	xsdRestriction * m_rest;
	xsdList        * m_list;
	xsdUnion       * m_union;
};

class xsdSequence : public xsdType
{
public:
	xsdSequence(xsdType * parent) : xsdType("",type_sequence,parent)
	{

	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);

	xsdElementList m_elements ;
	xsdTypeList    m_types ;
	std::string    m_structname;
};

class xsdGroup : public xsdType
{
public:
	xsdGroup(const char * name,xsdType * parent) : xsdType(name,type_group,parent)
	{
		m_type = NULL;
	}

	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);

	xsdType * m_type ;
};

class xsdGroupList : public std::list<xsdGroup*>
{
public:
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
};

typedef xsdGroupList::iterator groupIterator ;



class xsdSequenceList : public std::list<xsdSequence*>
{
public:
	void CalcDependency(xsdTypeList & list);
};

typedef xsdSequenceList::iterator sequenceIterator ;

class xsdChoice : public xsdType
{
public:
	xsdChoice(int min,int max,xsdType * parent) : xsdType("",type_choice,parent)
	{
		m_minOccurs = min ;
		m_maxOccurs = max ;
	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);

	int            m_minOccurs;
	int            m_maxOccurs;
	xsdElementList m_elements;
	xsdGroupList   m_groups;
	xsdChoiceList  m_choises;
	xsdSequenceList m_sequences;
};

class xsdAll : public xsdType
{
public:
	xsdAll(xsdType * parent) : xsdType("",type_all,parent)
	{

	}

	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);

	xsdElementList m_elements ;
};

class xsdComplexType : public xsdType
{
public:
	xsdComplexType(const char * name,const char * elemname,typetag tag,xsdType * parent) : xsdType(name,tag,parent)
	{
		if (m_name.empty() && *elemname != 0)
		{
			m_name = "c_";
			m_name += elemname;
			m_cname = MakeIdentifier("",m_name.c_str());
		}
		m_type = NULL;
	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent);
	void GenImpl(CppFile & out,Symtab & st);
	xsdType *   m_type;
	xsdAttrList m_attributes;
};

class xsdSchema
{
public:
	xsdSchema()
	{

	}
};

#endif // XSDCLASSGEN_H_INCLUDED
