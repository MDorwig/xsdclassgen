
#include <list>
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

typedef std::list<xsdType*>::iterator typeIterator;

typedef std::list<xsdChoice*> xsdChoiceList ;
typedef xsdChoiceList::iterator choiceIterator ;

std::string MakeIdentifier(const char * prefix,const char * name)
{
	std::string str ;
	if (prefix != NULL)
		str += prefix ;
	while(*name)
	{
		char c = *name++;
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

	void GenCode(FILE * out,int indent,bool choice);

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
	void GenCode(FILE * out,int indent,bool choice);
};

class xsdElementList : public std::list<xsdElement*>
{
public:
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent,bool choice);
};

typedef xsdElementList::iterator elementIterator ;


class xsdAttribute
{
public:
	xsdAttribute(const char * name,const char * defval,xsdType * type)
	{
		m_name    = name ;
		m_cname   = MakeIdentifier("a_",name);
		m_type    = type;
		m_default = defval;
	}
	std::string m_name ;
	std::string m_cname;
	xsdType * m_type ;
	std::string m_default;
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent);
};

class xsdAttrList : public std::list<xsdAttribute*>
{
public:
	void GenCode(FILE * out,int indent);
};

typedef xsdAttrList::iterator attrIterator ;


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
		std::string s = supertypeename;
		if (s.empty())
		{
			if (strncmp(elemname,"m_",2) == 0)
				elemname+=2 ;
			s = prefix;
			s += elemname;
		}
		return s ;
	}

	bool isAnonym()
	{
		return *getName() == 0 ;
	}
	virtual void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	virtual void GenCode(FILE * out,int indent);
	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	typetag m_tag;
	int     m_id ;
	bool    m_impl;
	bool    m_indeplist;
	static int m_count;
};

class xsdList : public xsdType
{
public:
	xsdList(const char * itemTypename,xsdType * type) : xsdType("",type_list)
	{
		m_typename = NULL;
		if (itemTypename != NULL)
			m_typename = new xsdTypename(itemTypename);
		m_type = type ;
	}
	void CalcDependency(xsdTypeList & list);
	xsdTypename * m_typename ;
	xsdType     * m_type ;

	void GenCode(FILE * out,int indent,const char * elemname,const char * simplename);

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
		m_base = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent,const char * elemname,const char * simplename);

	xsdType * m_base ;
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

class xsdExtension : public xsdType
{
public:
	xsdExtension(xsdTypename * basetypename) : xsdType("",type_extension)
	{
		m_basetypename = basetypename;
	}
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
//	void GenCode(FILE * out,int indent);

	xsdTypename * m_basetypename ;
	xsdAttrList   m_attributes;
};

class xsdSimpleContent : public xsdType
{
public:
	xsdSimpleContent() : xsdType("",type_simpleContent)
	{
		m_content = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	xsdType * m_content; // xsd_Restriction oder xsd_Extension
};

class xsdSimpleType : public xsdType
{
public:
	xsdSimpleType(const char * name) : xsdType(name,type_simple)
	{
		m_derived = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	xsdType * m_derived;
};

class xsdSequence : public xsdType
{
public:
	xsdSequence() : xsdType("",type_sequence)
	{

	}

	void CalcDependency(xsdTypeList & list);

	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	void GenCode(FILE * out,int indent);
	xsdElementList m_elements ;
	xsdTypeList    m_types ;
};

class xsdGroup : public xsdType
{
public:
	xsdGroup(const char * name) : xsdType(name,type_group)
	{
		m_type = NULL;
	}

	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent, const char * elemname,const char * supertypename);
	void GenCode(FILE * out,int indent);

	xsdType * m_type ;
};

class xsdGroupList : public std::list<xsdGroup*>
{
public:
	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent, const char * elemname,const char * supertypename);
	void GenCode(FILE * out,int indent);
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
	xsdChoice(int min,int max) : xsdType("",type_choice)
	{
		m_minOccurs = min ;
		m_maxOccurs = max ;
	}

	void CalcDependency(xsdTypeList & list);

	void GenCode(FILE * out,int indent,const char * elemname,const char * supertypename);
	void GenCode(FILE * out,int indent);
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
	xsdAll() : xsdType("",type_all)
	{

	}

	void CalcDependency(xsdTypeList & list);
	void GenCode(FILE * out,int indent, const char * elemname,const char * supertypename);
	void GenCode(FILE * out,int indent);

	xsdElementList m_elements ;
};

class xsdComplexType : public xsdType
{
public:
	xsdComplexType(const char * name,typetag tag) : xsdType(name,tag)
	{
		m_type = NULL;
	}

	void CalcDependency(xsdTypeList & list);

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
