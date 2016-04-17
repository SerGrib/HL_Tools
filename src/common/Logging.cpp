#include <cassert>
#include <chrono>

#include "Logging.h"

namespace
{
class CNullLogListener final : public ILogListener
{
	void LogMessage( const LogType type, const char* const pszMessage ) override final
	{
		//Nothing.
	}
};

static CNullLogListener g_NullLogListener;

static CLogging g_Logging;
}

ILogListener* GetNullLogListener()
{
	return &g_NullLogListener;
}

CLogging& logging()
{
	return g_Logging;
}

CLogging::CLogging()
{
	SetLogListener( GetDefaultLogListener() );
}

CLogging::~CLogging()
{
	CloseLogFile();
}

void CLogging::SetLogListener( ILogListener* pListener )
{
	if( pListener )
	{
		m_pListener = pListener;
	}
	else
	{
		m_pListener = GetDefaultLogListener();
	}
}

void CLogging::Log( const LogType type, const char* const pszFormat, ... )
{
	assert( pszFormat != nullptr && *pszFormat );

	va_list list;

	va_start( list, pszFormat );

	VLog( type, pszFormat, list );

	va_end( list );
}

void CLogging::VLog( const LogType type, const char* const pszFormat, va_list list )
{
	assert( pszFormat != nullptr && *pszFormat );

	if( m_bInLog )
		return;

	m_bInLog = true;

	char szBuffer[ 8192 ];

	const int iRet = vsnprintf( szBuffer, sizeof( szBuffer ), pszFormat, list );

	if( iRet < 0 || static_cast<size_t>( iRet ) >= sizeof( szBuffer ) )
	{
		snprintf( szBuffer, sizeof( szBuffer ), "Log buffer too small for '%s'\n", pszFormat );
	}

	if( IsLogFileOpen() )
	{
		fprintf( m_pLogFile, "%s", szBuffer );
	}

	GetLogListener()->LogMessage( type, szBuffer );

	m_bInLog = false;
}

bool CLogging::OpenLogFile( const char* const pszFilename, const bool bAppend )
{
	assert( pszFilename && *pszFilename );

	CloseLogFile();

	m_pLogFile = fopen( pszFilename, bAppend ? "wa" : "w" );

	if( m_pLogFile )
	{
		auto now = std::chrono::system_clock::now();

		const time_t time = std::chrono::system_clock::to_time_t( now );

		//No newline because ctime's return value contains one.
		fprintf( m_pLogFile, "Log opened on %s", ctime( &time ) );
	}

	return IsLogFileOpen();
}

void CLogging::CloseLogFile()
{
	if( IsLogFileOpen() )
	{
		auto now = std::chrono::system_clock::now();

		const time_t time = std::chrono::system_clock::to_time_t( now );

		//No newline because ctime's return value contains one.
		fprintf( m_pLogFile, "Log closed on %s", ctime( &time ) );

		fclose( m_pLogFile );
		m_pLogFile = nullptr;
	}
}

void Message( const char* const pszFormat, ... )
{
	va_list list;

	va_start( list, pszFormat );

	logging().VLog( LogType::MESSAGE, pszFormat, list );

	va_end( list );
}

void Warning( const char* const pszFormat, ... )
{
	va_list list;

	va_start( list, pszFormat );

	logging().VLog( LogType::WARNING, pszFormat, list );

	va_end( list );
}

void Error( const char* const pszFormat, ... )
{
	va_list list;

	va_start( list, pszFormat );

	logging().VLog( LogType::ERROR, pszFormat, list );

	va_end( list );
}