
#ifndef SYMTAB_H_IMCLUDED
#define SYMTAB_H_IMCLUDED

#include <string>
#include <list>

std::string MakeIdentifier(const char * prefix,const char * name);

class Symbol
{
public:
	Symbol(const char * name,int enumval)
	{
		m_name = name ;
		m_cname = MakeIdentifier("",name);
		m_enumval = enumval;
	}
	int operator < (Symbol & s);
	std::string m_name ;
	std::string m_cname ;
	int m_enumval;
};

class Symtab : public	std::list<Symbol*>
{
public:
	Symtab()
	{
		m_nextenumval = 0 ;
		m_hasAttributes = false;
	}

	Symbol * find(const char * name);
	void     add(const char * name);
	void     sortbyname();
	int 		 m_nextenumval ;
	bool     m_hasAttributes;
};

#endif
