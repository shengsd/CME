#include "stdafx.h"
#include "FieldType.h"

namespace MDP
{
	DateTime DateTime::nowUtc()
	{
#if defined( HAVE_FTIME )
		timeb tb;
		ftime (&tb);
		return fromUtcTimeT (tb.time, tb.millitm);
#elif defined( _POSIX_SOURCE )
		struct timeval tv;
		gettimeofday (&tv, 0);
		return fromUtcTimeT( tv.tv_sec, tv.tv_usec / 1000 );
#else
		return fromUtcTimeT( ::time (0), 0 );
#endif
	}

	DateTime DateTime::nowLocal()
	{
#if defined( HAVE_FTIME )
		timeb tb;
		ftime (&tb);
		return fromLocalTimeT( tb.time, tb.millitm );
#elif defined( _POSIX_SOURCE )
		struct timeval tv;
		gettimeofday (&tv, 0);
		return fromLocalTimeT( tv.tv_sec, tv.tv_usec / 1000 );
#else
		return fromLocalTimeT( ::time (0), 0 );
#endif
	}
}