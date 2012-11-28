class xsdType ;

#include <list>
#include <string>

class xsdElement
{
public:
	xsdElement(const char * name,const char * ftypename,xsdType * type)
	{
		m_name = name;
		m_typename = ftypename;
		m_type = type ;
		m_minOccurs=0;
		m_maxOccurs=0;
	}

	~xsdElement()
	{
	}

	const char * getName()
	{
		return m_name.c_str();
	}

	void GenCode(FILE * out,int indent);

	std::string m_name ;
	std::string m_typename;
	xsdType *  m_type ;
	int         m_minOccurs;
	int         m_maxOccurs;
};

typedef std::list<xsdElement*>::iterator elementIterator ;

class xsdElementList : public std::list<xsdElement *>
{

} ;

class xsdEnum
{
	char * m_Value ;
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
};

typedef std::list<xsdType*>::iterator typeListIterator;

class xsdTypeList : public std::list<xsdType*>
{
public:
	xsdTypeList()
	{
	}

	void Add(xsdType * type) ;
	xsdType * Find(const char * name);
};


class xsdType
{
public:
	xsdType(const char * name,typetag tag,bool builtin = false)
	{
		if (name == NULL)
			name = "";
		m_name = name;
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
	virtual void GenCode(FILE * out,int indent,const char * elemname);
	std::string m_name ;
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
	std::list<std::string> m_enumvalues;
	int m_whiteSpace;
};

class xsdSimpleType : public xsdType
{
public:
	xsdSimpleType(const char * name) : xsdType(name,type_simple)
	{
		m_rest = NULL;
	}
	void GenCode(FILE * out,int indent,const char * elemname);
	union
	{
		xsdRestriction * m_rest;
	};
};

class xsdSequenceType : public xsdType
{
public:
	xsdSequenceType() : xsdType("",type_sequence)
	{

	}
	void GenCode(FILE * out,int indent,const char * elemname);
	xsdElementList m_list ;
};

class xsdChoiceType : public xsdType
{
public:
	void GenCode(FILE * out,int indent);
};

class xsdComplexType : public xsdType
{
public:
	xsdComplexType(const char * name,typetag tag) : xsdType(name,tag)
	{
		m_sequence = NULL;
		m_choice   = NULL;
	}
	void GenCode(FILE * out,int indent,const char * elemname);
	union
	{
		xsdSequenceType * m_sequence;
		xsdChoiceType   * m_choice;
	};
};

class xsdSchema
{
public:
	xsdSchema()
	{

	}
};
