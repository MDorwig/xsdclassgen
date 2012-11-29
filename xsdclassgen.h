
#include <list>
#include <string>

class xsdElement;
class xsdType ;
class xsdChoice;
class xsdGroup;
class xsdSequence;
class xsdAttribute;

typedef std::list<xsdType*>::iterator typeIterator;

typedef std::list<xsdElement*> xsdElementList ;
typedef xsdElementList::iterator elementIterator ;

typedef std::list<xsdChoice*> xsdChoiceList ;
typedef xsdChoiceList::iterator choiceIterator ;

typedef std::list<xsdGroup*> xsdGroupList ;
typedef xsdGroupList::iterator groupIterator ;

typedef std::list<xsdSequence*> xsdSequenceList ;
typedef xsdSequenceList::iterator sequenceIterator ;

typedef std::list<xsdAttribute*> xsdAttrList ;
typedef xsdAttrList::iterator attrIterator ;

std::string MakeIdentifier(const char * prefix,const char * name)
{
	std::string str ;
	if (prefix != NULL)
		str += prefix ;
	while(*name)
	{
		char c = *name++;
		if (!isalnum(c) && c != '_')
			c = '_' ;
		str += c ;
	}
	return str ;
}

class xsdElement
{
public:
	xsdElement(const char * name,const char * ftypename,xsdType * type)
	{
		if (ftypename == NULL)
			ftypename = "";
		if (strchr(name,':') != NULL)
		{
			while(*name != ':')
				m_ns += *name++;
			name++;
		}
		m_name       = name;
		m_cname      = MakeIdentifier("m_",name);
		m_typename   = ftypename;
		m_type       = type ;
		m_minOccurs  = 0;
		m_maxOccurs  = 0;
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

	void GenCode(FILE * out,int indent);

	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	std::string m_typename;
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
};

class xsdAttribute
{
public:
	xsdAttribute() {}
	std::string m_name ;
	xsdType * m_type ;
	std::string m_default;
};

class xsdType
{
public:
	xsdType(const char * name,typetag tag,bool builtin = false)
	{
		if (name == NULL)
			name = "";
		if (strchr(name,':') != NULL)
		{
			while(*name != ':')
				m_ns += *name++;
			name++;
		}
		m_name = name;
		m_cname = MakeIdentifier(NULL,name);
		m_tag = tag ;
		m_id = ++m_count;
		m_impl = builtin ;
		m_indeplist = false ;
	}

	virtual ~xsdType()
	{
	}

	const char * getName()
	{
		return m_name.c_str();
	}

	const char * getCppName() ;

	bool isAnonym()
	{
		return *getName() == 0 ;
	}
	virtual void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	typetag m_tag;
	int     m_id ;
	bool    m_impl;
	bool    m_indeplist;
	static int m_count;
};

class xsdRestriction: public xsdType
{
public:
	xsdRestriction() : xsdType("",type_restriction)
	{
		m_minExclusive = 0 ;
		m_minInclusive = 0 ;
		m_maxExclusive = 0 ;
		m_maxInclusive = 0 ;
		m_totalDigits = 0 ;
		m_fractionDigits = 0 ;
		m_length = 0 ;
		m_minLength = 0 ;
		m_maxLength = 0 ;
		m_whiteSpace = 0 ;
	}
	void GenCode(FILE * out,int indent,const char * elemname,const char * simplename);

	std::string m_base ;
	int m_minExclusive;
	int m_minInclusive;
	int m_maxExclusive;
	int m_maxInclusive;
	int m_totalDigits;
	int m_fractionDigits;
	int m_length;
	int m_minLength;
	int m_maxLength;
	std::list<xsdEnumValue*> m_enumvalues;
	int m_whiteSpace;
};

class xsdSimpleType : public xsdType
{
public:
	xsdSimpleType(const char * name) : xsdType(name,type_simple)
	{
		m_rest = NULL;
	}
	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	union
	{
		xsdRestriction * m_rest;
	};
};

class xsdSequence : public xsdType
{
public:
	xsdSequence() : xsdType("",type_sequence)
	{

	}
	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	xsdElementList m_list ;
};

class xsdChoice : public xsdType
{
public:
	xsdChoice() : xsdType("",type_choice)
	{

	}

	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	xsdElementList m_elements;
	xsdGroupList   m_groups;
	xsdChoiceList  m_choises;
	xsdSequenceList m_sequences;
};

class xsdAll : public xsdType
{
public:
	xsdAll() : xsdType("",type_all)
	{

	}

	void GenCode(FILE * out,int indent, const char * elemname,const char * supertypename);

	xsdElementList m_elements ;
};

class xsdComplexType : public xsdType
{
public:
	xsdComplexType(const char * name,typetag tag) : xsdType(name,tag)
	{
		m_type = NULL;
	}
	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	xsdType * m_type;
	xsdAttrList m_attributes;
};

class xsdSchema
{
public:
	xsdSchema()
	{

	}
};
