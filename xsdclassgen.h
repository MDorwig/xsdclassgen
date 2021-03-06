
#ifndef XSDCLASSGEN_H_INCLUDED
#define XSDCLASSGEN_H_INCLUDED

#include <list>
#include <string.h>
#include <string>
#include <assert.h>

class xsdElement;
class xsdType ;
class xsdTypename;
class xsdChoice;
class xsdGroup;
class xsdSequence;
class xsdAttribute;
class xsdTypeList;
class xsdAttrList;
class xsdGroupList;
class xsdSimpleType;
class xsdNamespace;

#include "symtab.h"
#include "cppfile.h"

extern xsdNamespace * targetNamespace;
typedef std::list<xsdType*>::iterator typeIterator;

xsdType * FindType(const char * name);
xsdType * FindType(xsdTypename * tn);
std::string MakeIdentifier(const char * prefix,const char * name);

class xsdTypename // für Vorwärtsdeklarationen
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

class xsdEnumValue
{
public:
	xsdEnumValue(const char * value)
	{
		m_Value  = value;
		m_eValue = MakeIdentifier("e_",value);
		m_sValue = MakeIdentifier("",value);
	}
	const char * getName()
	{
		return m_Value.c_str();
	}

	const char * getEnumValue()
	{
		return m_eValue.c_str();
	}

	const char * getSymbolName()
	{
		return m_sValue.c_str();
	}

	std::string m_Value;
	std::string m_eValue;
	std::string m_sValue;
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
	type_simpleExtension,
	type_complexExtension,
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
	type_complexContent,
	type_simple,
	type_simpleRestriction,
	type_complexRestriction,
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
		listno = ++nextlistno;
	}

	xsdType * Find(const char * name);
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr,bool choice);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st);
	static int nextlistno;
	int  listno;
};

class xsdElementList : public std::list<xsdElement*>
{
public:
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
//	void GenImpl(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st);
	void GenInit(CppFile & out,int indent);
	void GenDelete(CppFile & out,int indent);
	bool CheckCycle(xsdElement * elem);
	bool hasPointer();
};

typedef xsdElementList::iterator elementIterator ;

class xsdAttrElemBase
{
public:
	xsdAttrElemBase(const char * name,const char * defval,const char * typname,const char * prefix)
	{
		if (strchr(name,':') != NULL)
		{
			while(*name != ':')
				m_ns += *name++;
			name++;
		}
		m_name    = name ;
		m_cname   = MakeIdentifier(prefix,name);
		m_type    = NULL;
		m_typename= NULL;
		if (typname != NULL)
		{
			m_typename = new xsdTypename(typname);
			m_type     = FindType(typname);
		}
		if (defval != NULL)
			m_default    = defval;
	}
	virtual ~xsdAttrElemBase()
	{
		if (m_typename != NULL)
			delete m_typename;
	}
	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	std::string m_default;
	xsdTypename * m_typename;
	xsdType *   m_type ;

	const char * getName() { return m_name.c_str();}
	const char * getCppName() { return m_cname.c_str();}
	virtual bool isPtr() { return false;}
	virtual bool isChoice() { return false;}
	virtual void CalcDependency(xsdTypeList & list) {}
	virtual void GenHeader(CppFile & out,int indent){}
	virtual const char * getlvalue() { return getCppName();}
	virtual const char * getChoiceVarname() { return "";}
	virtual const char * getChoiceTypename() { return "";}
	const char * getDefault() { return m_default.c_str();	}

};

class xsdAttribute : public xsdAttrElemBase
/*
 * attributeGroup,
 * schema,
 * complexType,
 * restriction (both simpleContent and complexContent),
 * extension (both simpleContent and complexContent)
 */
{
public:
	enum eUse
	{
		eUse_Optional,
		eUse_Prohibited,
		eUse_Required,
	};

	xsdAttribute(const char * name,const char * typname,const char * defval,const char * fixed,eUse use) :
		xsdAttrElemBase(name,defval,typname,"a_")
	{
		m_use     = use;
		m_fixed   = fixed;
	}
	void GenHeader(CppFile & out,int indent);
	void GenLocal(CppFile & out,Symtab & st);
	void CalcDependency(xsdTypeList & list);
	void GenWrite(CppFile & out,int indent, xsdElement * elem);

	std::string m_fixed;
	eUse        m_use ;
};

class xsdAttrList : public std::list<xsdAttribute*>
{
public:
	~xsdAttrList()
	{

	}
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st);
	void GenWrite(CppFile & out,int indent,xsdElement * elem);
	xsdAttrList & operator += (xsdAttrList & l);
};

typedef xsdAttrList::iterator attrIterator ;


class xsdType
{
public:
	xsdType(const char * name,typetag tag,xsdType * parent,bool builtin = false)
	{
		if (parent == NULL)
			m_local = false ;
		else
			m_local = parent->m_local;
		if (name == NULL)
			name = "";
		if (name[0] != 0)
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
		m_cd  = false;
		m_id = ++m_count;
		m_hdrimpl = builtin ;
		m_cppimpl = builtin ;
		m_indeplist = false ;
	}

	virtual ~xsdType()
	{
	}

	virtual void CalcDependency(xsdTypeList & list)
	{
	}

	virtual bool	isSignedInteger();
	virtual bool  isUnsignedInteger();
	virtual bool  isInteger();
	virtual bool  isfloat();
	virtual bool  isString();
	virtual bool  isEnum();
	virtual bool  isScalar();
	virtual bool  isStruct();
	virtual bool  isSigned();

	//virtual int   getDim() { return 1 ;}

	const char * getName()
	{
		return m_name.c_str();
	}

	virtual void setCppName(const char * name)
	{
		m_cname = name ;
	}

	const char * getCppName() ;
	const char * getNativeName();

	std::string getQualifiedName();
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

	bool tascd()
	{
		bool res = m_cd ;
		if (m_cd == false)
			m_cd = true ;
		return res ;
	}

	bool tashdr()
	{
		bool res = m_hdrimpl;
		if (m_hdrimpl == false)
				m_hdrimpl = true ;
		return res;
	}

	bool tascpp()
	{
		bool res = m_cppimpl;
		if (m_cppimpl == false)
				m_cppimpl = true ;
		return res;
	}

	virtual void GenHeader(CppFile & out,int indent,const char * defaultstr);
	virtual void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	virtual void GenWrite(CppFile & out,Symtab & st);
	virtual void GenAttrImpl(CppFile & out,Symtab & st);
	virtual void GenWriteAttr(CppFile & out,int indent,xsdElement * elem);
	virtual void GenInitFixedAttr(CppFile & out,int indent);
	virtual void GenAttrHeader(CppFile & out,int indent);
	virtual void GenLocal(CppFile & out,Symtab & st,const char * defaultstr) {}
	virtual void GenAssignment(CppFile & out,int indent,xsdAttrElemBase & elem,const char * src);
	virtual void GenAssignment(CppFile & out,int indent,const char * dest,const char * src);
	virtual bool CheckCycle(xsdElement * elem) { return false;}
	virtual bool hasAttributes() { return !m_attributes.empty(); }
	virtual void GetAttributes(xsdAttrList & lst) { lst = m_attributes;}
	bool         HasFixedAttributes();
	std::string m_ns;
	std::string m_name ;
	std::string m_cname;
	xsdType *   m_parent;
	bool        m_local;
	typetag 		m_tag;
	int     		m_id ;
	bool    		m_indeplist;
	xsdAttrList m_attributes;
private:
	bool        m_cd;
	bool    		m_hdrimpl;
	bool    		m_cppimpl;
	static int 	m_count;
};

class xsdEnum : public xsdType
{
public:
	xsdEnum(xsdType * parent) : xsdType("",type_enum,parent)
	{
		m_cname = "xs_enum";
	}

	void AddValue(xsdEnumValue * val)
	{
		m_values.push_back(val);
	}

	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	bool CheckCycle(xsdElement * elem);

	void setCppName(const char * name)
	{
		m_cname = name;
	}

	xsdEnumValueList m_values;
};


class xsdList : public xsdType
{
/*
 * <list
 * id = ID
 * itemType = QName
 * {any attributes with non-schema Namespace}...>
 *	Content: (annotation?, (simpleType?))
 * </list>
 */
public:
	xsdList(const char * itemTypename,xsdType * type,xsdType * parent) : xsdType("",type_list,parent)
	{
		m_itemtypename = NULL;
		if (itemTypename != NULL)
			m_itemtypename = new xsdTypename(itemTypename);
		m_itemtype = type ;
	}
	void CalcDependency(xsdTypeList & list);
	xsdTypename * m_itemtypename ;
	xsdType     * m_itemtype ;

	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src);
	void setCppName(const char *name);
	int  getDim();
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

class xsdRestictionValue
{
public:
	xsdRestictionValue()
	{
		m_value = 0 ;
		m_bset  = false ;
	}

	void set(int value)
	{
		m_value= value;
		m_bset = true;
	}

	int get() const   {return m_value;}
	int isset() const {return m_bset;}
private:
	int  m_value ;
	bool m_bset;
};

class xsdSimpleRestriction: public xsdType
{
public:
	xsdSimpleRestriction(const char * base,xsdType * parent) : xsdType("",type_simpleRestriction,parent)
	{
		m_base   = NULL;
		m_simple = NULL;
		m_enum   = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAssignment(CppFile & out,int indent,xsdAttrElemBase & elem,const char * src);
	bool hasAttributes()
	{
		return xsdType::hasAttributes() ||
				   (m_base != NULL && m_base->hasAttributes());
	}

	void GetAttributes(xsdAttrList & lst)
	{
		lst = m_attributes;
	}

	void GenWriteAttr(CppFile & out,int indent,xsdElement * elem)
	{
		xsdType::GenWriteAttr(out,indent,elem);
		if (m_base != NULL)
			m_base->GenWriteAttr(out,indent,elem);
	}
	void setCppName(const char * name);
	bool isInteger() { return m_base->isInteger();}
	bool isfloat()   { return m_base->isfloat();}
	bool isStruct();
	bool isString();
	bool isScalar();
	const char * getReturnType();

	xsdType       * m_base ;
	xsdEnum       * m_enum;
	xsdSimpleType * m_simple;
	xsdRestictionValue 	m_minExclusive;
	xsdRestictionValue 	m_minInclusive;
	xsdRestictionValue 	m_maxExclusive;
	xsdRestictionValue	m_maxInclusive;
	xsdRestictionValue	m_totalDigits;
	xsdRestictionValue	m_fractionDigits;
	xsdRestictionValue	m_length;
	xsdRestictionValue	m_minLength;
	xsdRestictionValue	m_maxLength;
	xsdRestictionValue	m_whiteSpace;
};

class xsdSimpleExtension : public xsdType
{
public:
	xsdSimpleExtension(const char * base,xsdType * parent) : xsdType("",type_simpleExtension,parent)
	{
		m_basetypename = new xsdTypename(base);
		m_exttype = NULL;
		m_base = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAttrImpl(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st);
	bool hasAttributes()
	{
		return xsdType::hasAttributes() ||
				  (m_base != NULL && m_base->hasAttributes()) ||
			    (m_exttype != NULL && m_exttype->hasAttributes());
	}

	void GetAttributes(xsdAttrList & lst)
	{
		lst += m_attributes;
	}

	void GenWriteAttr(CppFile & out,int indent,xsdElement * elem)
	{
		if (m_base != NULL)
			m_base->GenWriteAttr(out,indent,elem);
		xsdType::GenWriteAttr(out,indent,elem);
		if (m_exttype != NULL)
			m_exttype->GenWriteAttr(out,indent,elem);
	}

	xsdTypename * m_basetypename ;
	xsdType     * m_base;
	xsdType     * m_exttype;
};

class xsdSimpleContent : public xsdType
{
public:
	xsdSimpleContent(xsdType * parent) : xsdType("",type_simpleContent,parent)
	{
		m_content = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	bool CheckCycle(xsdElement * elem);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	bool hasAttributes()
	{
		return xsdType::hasAttributes() ||
				   (m_content != NULL && m_content->hasAttributes());
  }

	void GetAttributes(xsdAttrList & lst)
	{
		if (m_content != NULL && m_content->hasAttributes())
			m_content->GetAttributes(lst);
		lst += m_attributes;
	}

	void GenWriteAttr(CppFile & out,int indent,xsdElement * elem)
	{
		if (m_content != NULL)
			m_content->GenWriteAttr(out,indent,elem);
		xsdType::GenWriteAttr(out,indent,elem);
	}

	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAttrImpl(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st);

	xsdType * m_content; // xsd_Restriction oder xsd_Extension
};

class xsdSimpleType : public xsdType
{
public:
	xsdSimpleType(const char * name,const char * elemname,xsdType * parent) : xsdType(name,type_simple,parent)
	{
		if (m_name.empty())
		{
			if (elemname != NULL && elemname[0])
			{
				/*
				 * simpleType ohne namen aber mit zugehörigem element
				 * es wird ein typname in der form "<elemname>_t" erzeugt
				 */
				m_name = elemname;
				m_name += "_t";
				m_cname = MakeIdentifier("",m_name.c_str());
			}
			m_local= true;
		}
		m_rest = NULL;
		m_list = NULL;
		m_union= NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAssignment(CppFile & out,int indent,xsdAttrElemBase & leftside,const char * rightside);
	int  getDim();
	void setCppName(const char * name);

	xsdSimpleRestriction * m_rest;
	xsdList        * m_list;
	xsdUnion       * m_union;
};

class xsdSequence : public xsdType
{
public:
	xsdSequence(xsdType * parent) : xsdType("",type_sequence,parent)
	{
		m_maxOccurs = 1 ;
		m_minOccurs = 1 ;
	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	bool CheckCycle(xsdElement * elem);

	int            m_maxOccurs;
	int            m_minOccurs;
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
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	bool CheckCycle(xsdElement * elem);

	xsdType * m_type ;
};

class xsdGroupList : public std::list<xsdGroup*>
{
public:
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
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
	xsdChoice(int seq,int min,int max,xsdType * parent) : xsdType("",type_choice,parent)
	{
		char tmp[16];
		m_minOccurs = min ;
		m_maxOccurs = max ;
		m_seq       = seq;
		sprintf(tmp,"xs_choice_%d",seq);
		m_typename = tmp ;
		sprintf(tmp,"m_choice_%d",seq);
		m_varname = tmp ;
	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	bool CheckCycle(xsdElement * elem);
	const char * getVarname()
	{
		return m_varname.c_str();
	}

	const char * getTypename()
	{
		return m_typename.c_str();
	}

	int            m_minOccurs;
	int            m_maxOccurs;
	int            m_seq ;
	xsdElementList m_elements;
	xsdGroupList   m_groups;
	xsdSequenceList m_sequences;
	std::string     m_varname;
	std::string     m_typename;
};


class xsdAll : public xsdType
{
public:
	xsdAll(xsdType * parent) : xsdType("",type_all,parent)
	{

	}

	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	bool CheckCycle(xsdElement * elem);
	xsdElementList m_elements ;
};

class xsdElement : public xsdAttrElemBase
{
public:
	xsdElement(const char * name,const char * typname,int minOccurs,int maxOccurs,xsdChoice * choice,const char * xsddefault) :
	  xsdAttrElemBase(name,xsddefault,typname,"m_")
	{
		m_choice     = choice;
		m_isCyclic   = false;
		if (m_choice)
		{
			m_choice_selector = MakeIdentifier("e_",name);
		}
		m_minOccurs  = minOccurs;
		m_maxOccurs  = maxOccurs;
		m_tns        = targetNamespace;
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


	bool isArray()
	{
		return m_maxOccurs != 1 ;
	}

	bool isUnbounded()
	{
		return m_maxOccurs == 0 ;
	}

	bool hasDefault()
	{
		return !m_default.empty();
	}

	const char * getChoiceTypename()
	{
		assert(m_choice != NULL);
		return m_choice->getTypename();
	}

	const char * getChoiceVarname()
	{
		assert(m_choice != NULL);
		return m_choice->getVarname();
	}

	const char * getlvalue()
	{
		static std::string lval ;
		if (isPtr())
		{
			lval = "(*";
			if (isChoice())
			{
				lval += getChoiceVarname();
				lval += ".";
			}
			lval += getCppName();
			lval += ")";
		}
		else
		{
			lval = getCppName();
		}
		return lval.c_str();
	}

	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent);
	void GenRootHeader(CppFile & out,int indent);
	void GenRootImpl(CppFile & out,Symtab & st);
//	void GenImpl(CppFile & out,Symtab & st);
	void GenWrite(CppFile & out);
	void GenWriteAttr(CppFile & out,int indent);
	void GenLocal(CppFile & out,Symtab & st);
	void GenInit(CppFile & out,int indent);
	void GenDelete(CppFile & out,int indent);
	bool isPtr() { return (m_choice != NULL || m_isCyclic) && !isArray();}
	bool isChoice() { return m_choice != NULL;}
	bool hasAttributes();
	std::string m_choice_selector;
	xsdNamespace* m_tns ;
	xsdTypeList m_deplist;
	xsdChoice  *m_choice;
	bool        m_isCyclic;
	unsigned		m_minOccurs;
	unsigned   	m_maxOccurs;
};


class xsdComplexType : public xsdType
{
public:
	xsdComplexType(const char * name,xsdElement * elem,xsdType * parent) : xsdType(name,type_complex,parent)
	{
		m_rootelem = NULL;
		if (parent == NULL)
			m_rootelem = elem;
		if (m_name.empty())
		{
			if (elem != NULL)
			{
				/*
				 * simpleType ohne namen aber mit zugehörigem element
				 * es wird ein typname in der form "<elemname>_t" erzeugt
				 */
				m_name = elem->getName();
				m_name += "_t";
				m_cname = MakeIdentifier("",m_name.c_str());
			}
			m_local= true;
		}
		if (elem != NULL)
			elem->m_type = this;
		m_type = NULL;
	}

	void CalcDependency(xsdTypeList & list);

	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAttrImpl(CppFile & out,Symtab & st);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenAssignment(CppFile & out,int indent,xsdAttrElemBase & dest,const char * src);
	void GenAssignment(CppFile & out,int indent,const char * dest,const char * src);
	bool CheckCycle(xsdElement * elem);
	bool hasAttributes()
	{
		return xsdType::hasAttributes() ||
				   (m_type != NULL && m_type->hasAttributes());
	}

	void GetAttributes(xsdAttrList & lst)
	{
		if (m_type != NULL && m_type->hasAttributes())
			m_type->GetAttributes(lst);
		lst += m_attributes;
	}

	void GenWriteAttr(CppFile & out,int indent,xsdElement * elem)
	{
		if (m_type != NULL)
			m_type->GenWriteAttr(out,indent,elem);
		xsdType::GenWriteAttr(out,indent,elem);
	}
	/* (
	 *    simpleContent  |
	 *    complexContent |
	 *    ( all | choice | sequence), attribute)
	 */
	xsdType *   m_type;
	xsdElement* m_rootelem;
};

class xsdComplexRestriction : public xsdSimpleRestriction
{
public:
	xsdComplexRestriction(const char * basetype,xsdType * parent) :
		xsdSimpleRestriction(basetype,parent)
	{
		m_tag = type_complexRestriction ;
		m_type = NULL;
	}
	void GenWrite(CppFile & out,Symtab & st);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	xsdType * m_type ;
};

class xsdComplexExtension : public xsdSimpleExtension
{
public:
	xsdComplexExtension(const char * basetype,xsdType * parent) :
		xsdSimpleExtension(basetype,parent)
	{
		m_tag = type_complexExtension;
		m_type = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	void GenWrite(CppFile & out,Symtab & st)
	{
		if (m_type != NULL)
		{
			m_type->GenWrite(out,st);
		}
		else if (m_basetypename != NULL)
		{
			out.iprintln(1,"%s::Write(out);",m_basetypename->getCppName());
		}
	}

	xsdType * m_type ; /* group | all | choice | sequence */
};

class xsdComplexContent : public xsdType
{
public:
	xsdComplexContent(const char * name,const char * elemname,xsdType * parent) :
		xsdType(name,type_complexContent,parent)
	{
		m_type = NULL;
	}
	void CalcDependency(xsdTypeList & list);
	void GenHeader(CppFile & out,int indent,const char * defaultstr);
	void GenImpl(CppFile & out,Symtab & st,const char * defaultstr);
	void GenLocal(CppFile & out,Symtab & st,const char * defaultstr);
	bool hasAttributes()
	{
		return xsdType::hasAttributes() ||
					 (m_type != NULL && m_type->hasAttributes());
	}

	void GetAttributes(xsdAttrList & lst)
	{
		if (m_type != NULL)
			m_type->GetAttributes(lst);
		lst += m_attributes;
	}

	void GenWriteAttr(CppFile & out,int indent,xsdElement * elem)
	{
		if (m_type != NULL)
			m_type->GenWriteAttr(out,indent,elem);
		xsdType::GenWriteAttr(out,indent,elem);
	}

	void GenAttrImpl(CppFile & out,Symtab & st)
	{
		if (m_type != NULL)
			m_type->GenAttrImpl(out,st);
		else
			xsdType::GenAttrImpl(out,st);
	}
	void GenWrite(CppFile & out,Symtab & st)
	{
		if (m_type != NULL)
			m_type->GenWrite(out,st);
	}

	xsdType * m_type; // extension | restriction
};

class xsdSchema
{
public:
	xsdSchema()
	{

	}
};


void GenParserChildLoopStart(CppFile & out,xsdElementList & elements);
void GenElementCases(CppFile & out,Symtab & st,xsdElementList & elements,bool ischoice);
void GenWriteElementCases(CppFile & out,Symtab & st,xsdElementList & elements);
void GenWriteElements(CppFile & out,xsdElementList & elements);
void GenParserAttrLoop(CppFile & out,Symtab & st,xsdAttrList & attributes);
void GenParserChildLoopEnd(CppFile & out);


#endif // XSDCLASSGEN_H_INCLUDED
