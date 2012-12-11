
#ifndef CPPFILE_H_INCLUDED
#define CPPFILE_H_INCLUDED

#include <stdio.h>

class CppFile
{
public:
	CppFile(const char * filename)
	{
		m_filename = filename;
		m_file = NULL;
	}

	bool create()
	{
		m_file = fopen(m_filename.c_str(),"w+");
		return m_file != NULL;
	}
	void close()
	{
		if (m_file != NULL)
			fclose(m_file);
	}
	void 				Indent(int indent);
	int 				printf(const char * fmt,...)  __attribute__((format(printf,2,3)));
	int 				iprintf(int indent,const char * fmt,...)  __attribute__((format(printf,3,4)));
	int 				println(const char * fmt,...) __attribute__((format(printf,2,3)));
	int 				iprintln(int indent,const char * fmt,...) __attribute__((format(printf,3,4)));
	int         println() ;
	void 				GenSymtab(Symtab & st);
private:
	int         vprintf(const char * fmt,va_list lst);
	int         vprintln(const char * fmt,va_list lst);
	std::string m_filename;
	FILE * 			m_file;
} ;

#endif
