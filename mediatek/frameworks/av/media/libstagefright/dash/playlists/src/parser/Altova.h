////////////////////////////////////////////////////////////////////////
//
// Altova.h
//
// This file was generated by XMLSpy 2012r2sp1 Enterprise Edition.
//
// YOU SHOULD NOT MODIFY THIS FILE, BECAUSE IT WILL BE
// OVERWRITTEN WHEN YOU RE-RUN CODE GENERATION.
//
// Refer to the XMLSpy Documentation for further details.
// http://www.altova.com/xmlspy
//
////////////////////////////////////////////////////////////////////////


#ifndef ALTOVA_H_INCLUDED
#define ALTOVA_H_INCLUDED

#include "AltovaDefs.h"
#include <string>




#if defined(UNICODE) || defined(_UNICODE)
	#define tstring			std::wstring
	#define tstringstream	std::wstringstream
	#define tcin			std::wcin
	#define tcout			std::wcout
	#define tcerr			std::wcerr
	#define tclog			std::wclog
	#define tostream		std::wostream
	#define tofstream		std::wofstream
	#define tostringstream	std::wostringstream

	typedef std::wstring string_type;
#else
	#define tstring			std::string
	#define tstringstream	std::stringstream	
	#define tcin			std::cin
	#define tcout			std::cout
	#define tcerr			std::cerr
	#define tclog			std::clog
	#define tostream		std::ostream
	#define tofstream		std::ofstream
	#define tostringstream	std::ostringstream

	typedef std::string string_type;
#endif

typedef string_type::value_type char_type;


namespace altova {


////////////////////////////////////////////////////////////////////////
//
//  Utility functions
//
////////////////////////////////////////////////////////////////////////


#define ThrowFormatError() \
	throw CAltovaException(CAltovaException::eError1, _T("Format error"));

#define ThrowOutOfRangeError() \
	throw CAltovaException(CAltovaException::eError1, _T("Out of range!"));

#define ThrowIncompatibleTypesError() \
	throw CAltovaException(CAltovaException::eError1, _T("Types incompatible!"));

#define ThrowValuesNotConvertableError() \
	throw CAltovaException(CAltovaException::eError1, _T("Values are not convertable"));

#if defined( __GNUC__ )
	#define __int64 long long
	#define ALTOVA_INT64 long long 
    #define _I64_MIN LONG_LONG_MIN
    #define _I64_MAX LONG_LONG_MAX

#if defined(UNICODE) || defined(_UNICODE)
#define TCHAR       wchar_t
#define _T(x) 		L ## x

/* String conversion functions */

#define _tcstod     wcstod
#define _tcstol     wcstol
#define _tcstoul    wcstoul

/* String functions */

#define _tcscat     wcscat
#define _tcschr     wcschr
#define _tcscpy     wcscpy
#define _tcscspn    wcscspn
#define _tcslen     wcslen
#define _tcsncat    wcsncat
#define _tcsncpy    wcsncpy
#define _tcspbrk    wcspbrk
#define _tcsrchr    wcsrchr
#define _tcsspn     wcsspn
#define _tcsstr     wcsstr
#define _tcstok     wcstok

#define _tcscmp     wcscmp
#define _tcsnccmp   wcsncmp
#define _tcsncmp    wcsncmp

#define _tcscoll    wcscoll

#define _istspace   iswspace

#define _stscanf	swscanf
#define _totupper   towupper
#define _tfopen     altovawfopen
#else //NON UNICODE
#define TCHAR       char
#define _T(x) 		x

/* String conversion functions */

#define _tcstod     strtod
#define _tcstol     strtol
#define _tcstoul    strtoul

#define _tcschr     strchr
#define _tcscspn    strcspn
#define _tcsncat    strncat
#define _tcsncpy    strncpy
#define _tcspbrk    strpbrk
#define _tcsrchr    strrchr
#define _tcsspn     strspn
#define _tcsstr     strstr
#define _tcstok     strtok

#define _tcscmp     strcmp
#define _tcsnccmp   strncmp
#define _tcsncmp    strncmp

#define _tcscoll    strcoll

#define _istspace   isspace
#define _sntprintf  snprintf
#define _stscanf	sscanf
#define _totupper   toupper
#define _tfopen     fopen
#endif

#else
	#define ALTOVA_INT64 __int64
#endif

#ifdef _XERCES_VERSION

	#if defined(UNICODE) || defined(_UNICODE)
		#define XC2TS(x) x
	#else
		#define XC2TS(x) StrX(x).localForm()
	#endif

	class XStr
	{
	public:
		XStr(const char* const toTranscode) { m_bClone = true; fUnicodeForm = xercesc::XMLString::transcode(toTranscode); }
		XStr(const std::string& toTranscode) { m_bClone = true; fUnicodeForm = xercesc::XMLString::transcode(toTranscode.c_str()); }
		XStr(const wchar_t* const toTranscode) { m_bClone = false; fUnicodeForm = (XMLCh*)toTranscode; }
		XStr(const std::wstring& toTranscode) { m_bClone = false; fUnicodeForm = (XMLCh*)(toTranscode.c_str()); }
		~XStr() { if (m_bClone && fUnicodeForm) xercesc::XMLString::release(&fUnicodeForm); }
		const XMLCh* unicodeForm() const { return fUnicodeForm; }
	protected:
		bool m_bClone;
		XMLCh* fUnicodeForm;
	};

	class StrX
	{
	public:
		StrX(const XMLCh* const toTranscode) { fLocalForm = xercesc::XMLString::transcode(toTranscode); }
		~StrX() { xercesc::XMLString::release(&fLocalForm); }
		const char* localForm() const { return fLocalForm; }
	protected:
		char* fLocalForm;
	};

#endif

////////////////////////////////////////////////////////////////////////
//
//  CBaseObject
//
////////////////////////////////////////////////////////////////////////


class ALTOVA_DECLSPECIFIER CBaseObject
{
public:
	virtual ~CBaseObject() {}
};

} // namespace altova

#endif
