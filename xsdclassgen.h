class CxmlType ;

#include <list>
#include <string>

class CxmlElement
{
public:
	CxmlElement(const xmlChar * name,CxmlType * type,const xmlChar * ftypename)
	{
		m_name = (const char*)name;
		m_typename = (const char*)ftypename;
		m_type = type ;
		m_minOccurs=0;
		m_maxOccurs=0;
	}

	~CxmlElement()
	{
	}

	std::string m_name ;
	std::string m_typename;
	CxmlType *  m_type ;
	int         m_minOccurs;
	int         m_maxOccurs;
};

class CxmlEnum
{
	char * m_Value ;
};

enum typetag
{
	type_int,
	type_string,
	type_typename,
	type_complex,
	type_simple,
	type_restriction,
	type_sequence,
};

class CxmlType
{
public:
	CxmlType(const xmlChar * name,typetag tag)
	{
		m_name = (const char*)name;
		m_tag = tag ;
	}

	std::string m_name ;
	typetag m_tag;
};

class CxmlRestriction: public CxmlType
{
public:
	CxmlRestriction() : CxmlType(BAD_CAST "",type_restriction)
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

class CxmlSimpleType : public CxmlType
{
public:
	CxmlSimpleType(const xmlChar * name) : CxmlType(name,type_simple)
	{
		m_rest = NULL;
	}
	union
	{
		CxmlRestriction * m_rest;
	};
};

class CxmlSequenceType : public CxmlType
{
public:
	CxmlSequenceType() : CxmlType(BAD_CAST "",type_sequence)
	{

	}

	std::list<CxmlElement *> m_list ;
};

class CxmlChoiceType : public CxmlType
{
public:
};

class CxmlComplexType : public CxmlType
{
public:
	CxmlComplexType(const xmlChar * name,typetag tag) : CxmlType(name,tag)
	{
		m_sequence = NULL;
		m_choice   = NULL;
	}
	union
	{
		CxmlSequenceType * m_sequence;
		CxmlChoiceType   * m_choice;
	};
};

class CxmlSchema
{
public:
	CxmlSchema()
	{

	}
	void addElement(CxmlElement * e)
	{
		m_elements.push_back(e);
	}
	void addType(CxmlType * t)
	{
		m_types.push_back(t);
	}
	std::list<CxmlElement *> m_elements;
	std::list<CxmlType    *> m_types;
};
