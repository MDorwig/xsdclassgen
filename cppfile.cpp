/*
 * cppfile.cpp
 *
 *  Created on: 03.12.2012
 *      Author: dorwig
 */
#include <stdarg.h>
#include "xsdclassgen.h"

void CppFile::Indent(int indent)
{
	while(--indent >= 0)
	{
		fputs("  ",m_file); // 2 Blanks
		fflush(m_file);
	}
}

int CppFile::vprintf(const char * fmt,va_list lst)
{
	int n = 0 ;
	if (m_file != NULL)
		n = vfprintf(m_file,fmt,lst);
	return n ;
}
int CppFile::printf(const char * fmt,...)
{
	va_list lst ;
	va_start(lst,fmt);
	return vprintf(fmt,lst);
}

int CppFile::iprintf(int lvl,const char * fmt,...)
{
	va_list lst ;
	va_start(lst,fmt);
	Indent(lvl);
	return vprintf(fmt,lst);
}

int CppFile::println()
{
	int n = 0 ;
	if (m_file != NULL)
	{
		fputc('\n',m_file);
		n = 1;
	}
	return n ;
}

int CppFile::vprintln(const char * fmt,va_list lst)
{
	int n;
	n = vprintf(fmt,lst);
	n += println();
	return n ;
}

int CppFile::println(const char * fmt,...)
{
	va_list lst ;
	va_start(lst,fmt);
	return vprintln(fmt,lst);
}


int CppFile::iprintln(int lvl,const char * fmt,...)
{
	va_list lst ;
	va_start(lst,fmt);
	Indent(lvl);
	return vprintln(fmt,lst);
}

void CppFile::GenSymtab(Symtab & symtab)
{
	/*
	 * generate a enum with all names
	 */
	println("enum symbols {");
	println("  _nosymbol,");
	for (Symtab::iterator si = symtab.begin() ; si != symtab.end() ; si++)
	{
		println("  sy_%s,",(*si)->m_cname.c_str());
	}
	println("};");
	println();
	/*
	 * generate the lookuptable
	 */
	println("struct symbolentry {");
	println("  const char * name;");
	println("        symbols value;");
	println("};");
	println("static symbolentry symtab[] = ");
	println(" {");
	for (Symtab::iterator si = symtab.begin() ; si != symtab.end() ; si++)
	{
		println("  {\"%s\",sy_%s},",(*si)->m_name.c_str(),(*si)->m_cname.c_str());
	}
	println(" };");
	println();
	/*
	 * generate the compare function
	 */
	println("static int symcompare(const void * k,const void * e)");
	println("{");
	println("  const char * key = (const char *)k;") ;
	println("  const symbolentry * s = (symbolentry*)e;");
	println("  return strcmp(key,s->name);");
	println("}");
	println();
	/*
	 * generate the Lookup function
	 */
	println("static symbols Lookup(const xmlChar * name)");
	println("{");
	println("  symbols sy = _nosymbol;");
	println("  symbolentry * se = (symbolentry *)bsearch(name,symtab,sizeof symtab/sizeof symtab[0],sizeof symtab[0],symcompare);");
	println("  if (se != NULL)");
	println("    sy = se->value;");
	println("  return sy;");
	println("}\n");

	if (symtab.m_hasAttributes)
	{
		println("static const char * getContent(xmlAttrPtr attr)\n"
						"{\n"
						"  const char * content = \"\";\n"
						"  if (attr->children != NULL)\n"
						"    content = (const char*)attr->children->content;\n"
						"  return content;\n"
						"}\n");
	}
	println("static const char * getContent(xmlNodePtr child)\n"
			    "{\n"
				  "  const char * content = \"\";\n"
			    "  if (child->children != NULL)\n"
			    "    content = (const char*)child->children->content;\n"
			    "  return content;\n"
					"}\n\n");
#if 0
	println("static char * mystrtok(char ** cp,const char * delim)\n"
	        "{\n"
				  "  char * start = *cp;\n"
		      "  char * end;\n"
			    "  if (*start == 0)\n"
	        "    return NULL;\n"
          "  end = strstr(start,delim);\n"
		      "  if (end != NULL)\n"
			    "    *end++ = 0;\n"
		      "  else\n"
			    "    end = start+strlen(start);\n"
		      "  *cp = end ;\n"
		      "  return start ;\n"
	        "}\n");
#endif
}

