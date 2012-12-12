/*
 * symtab.cpp
 *
 *  Created on: 03.12.2012
 *      Author: dorwig
 */

#include <string.h>
#include "symtab.h"

bool compare_by_name(Symbol * s1,Symbol * s2)
{
	const char * str1 ;
	const char * str2 ;
	return s1->m_name < s2->m_name;
}

Symbol * Symtab::find(const char * name)
{
	for (Symtab::iterator si = begin() ; si != end() ; si++)
	{
		Symbol * s = *si ;
		if (s->m_name == name)
			return s ;
	}
	return NULL;
}

void Symtab::sortbyname()
{
	sort(compare_by_name);
}

void Symtab::add(const char * name)
{
	Symbol * s = find(name);
	if (s == NULL)
	{
		s = new Symbol(name,++m_nextenumval);
		push_back(s);
	}
}
