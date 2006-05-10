/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Patrick Luby, May 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 by Patrick Luby (patrick.luby@planamesa.com)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

#include "quicktimeplayer.hxx"

#define AVMEDIA_QUICKTIME_PLAYER_IMPLEMENTATIONNAME "com.sun.star.comp.avmedia.Player_QuickTime"
#define AVMEDIA_QUICKTIME_PLAYER_SERVICENAME "com.sun.star.media.Player_QuickTime"

using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::media;
using namespace ::com::sun::star::uno;

namespace avmedia
{
namespace quicktime
{

Player::Player( const Reference< XMultiServiceFactory >& rxMgr ) :
	mbLooping( false ),
	mxMgr( rxMgr ),
	maMovie( NULL ),
	maMovieGWorld( NULL ),
	mbRunning( false )
{
}

// ============================================================================

Player::~Player()
{
	stop();

	if ( maMovie )
		DisposeMovie( maMovie );

	if ( maMovieGWorld )
		DisposeGWorld( maMovieGWorld );
}

// ----------------------------------------------------------------------------

bool Player::create( const ::rtl::OUString& rURL )
{
	bool bRet = false;

	stop();

	if ( maMovie )
	{
		DisposeMovie( maMovie );
		maMovie = NULL;
	}

	mbLooping = false;
	mbRunning = false;

	::rtl::OString aURLBytes( rURL.getStr(), rURL.getLength(), RTL_TEXTENCODING_UTF8 );
	CFURLRef aURL = CFURLCreateWithBytes( NULL, (const UInt8 *)aURLBytes.getStr(), aURLBytes.getLength(), kCFStringEncodingUTF8, NULL );
	if ( aURL )
	{
		Handle hHandle = NewHandle( sizeof( AliasHandle ) );
		if ( hHandle )
		{
			OSType nType;
			if ( QTNewDataReferenceFromCFURL( aURL, 0, &hHandle, &nType ) == noErr )
			{
				if ( !maMovieGWorld )
				{
					Rect aRect;
					aRect.left = 0;
					aRect.top = 0;
					aRect.right = 1;
					aRect.bottom = 1;
					NewGWorld( &maMovieGWorld, 32, &aRect, NULL, NULL, 0 );
				}

				if ( maMovieGWorld && EnterMovies() == noErr )
				{
					short nID = movieInDataForkResID;
					OSErr nErr = NewMovieFromDataRef( &maMovie, 0, &nID, hHandle, nType );
					if ( nErr == noErr )
					{
						SetMovieGWorld( maMovie, maMovieGWorld, NULL );
						MoviesTask( maMovie, 0 );
						bRet = true;
					}
				}
			}

			DisposeHandle( hHandle );
		}

		CFRelease( aURL );
	}

	return bRet;
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::start() throw( RuntimeException )
{
	if ( maMovie )
	{
		StartMovie( maMovie );
		mbRunning = true;
	}
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::stop() throw( RuntimeException )
{
	if ( maMovie )
	{
		StopMovie( maMovie );
		mbRunning = false;
	}
}

// ----------------------------------------------------------------------------

sal_Bool SAL_CALL Player::isPlaying() throw( RuntimeException )
{
	if ( maMovie && mbRunning && !mbLooping && IsMovieDone( maMovie ) )
		mbRunning = false;

	return mbRunning;
}

// ----------------------------------------------------------------------------

double SAL_CALL Player::getDuration() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::getDuration not implemented\n" );
#endif
	double aRefTime( 0.0 );
	return aRefTime;
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::setMediaTime( double fTime ) throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::setMediaTime not implemented\n" );
#endif
}

// ----------------------------------------------------------------------------

double SAL_CALL Player::getMediaTime() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::getMediaTime not implemented\n" );
#endif
	double aRefTime( 0.0 );
	return aRefTime; 
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::setStopTime( double fTime ) throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::setStopTime not implemented\n" );
#endif
}

// ----------------------------------------------------------------------------

double SAL_CALL Player::getStopTime() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::getStopTime not implemented\n" );
#endif
	double aRefTime( 0.0 );
	return aRefTime; 
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::setRate( double fRate ) throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::setRate not implemented\n" );
#endif
}

// ----------------------------------------------------------------------------

double SAL_CALL Player::getRate() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::getRate not implemented\n" );
#endif
	double fRet( 0.0 );
	return fRet;
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::setPlaybackLoop( sal_Bool bSet ) throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::setPlaybackLoop not implemented\n" );
#endif
}

// ----------------------------------------------------------------------------

sal_Bool SAL_CALL Player::isPlaybackLoop() throw( RuntimeException )
{
	return mbLooping;
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::setMute( sal_Bool bSet ) throw( RuntimeException )
{
	if ( maMovie )
	{
		short nVolume = GetMovieVolume( maMovie );
		if ( bSet && nVolume > kNoVolume )
			SetMovieVolume( maMovie, nVolume * -1 );
		else if ( !bSet && nVolume < kNoVolume )
			SetMovieVolume( maMovie, nVolume * -1 );
	}
}

// ----------------------------------------------------------------------------

sal_Bool SAL_CALL Player::isMute() throw( RuntimeException )
{
	if ( maMovie && GetMovieVolume( maMovie ) < kNoVolume )
		return true;
	else
		return false;
}

// ----------------------------------------------------------------------------

void SAL_CALL Player::setVolumeDB( sal_Int16 nVolumeDB ) throw( RuntimeException )
{
	if ( maMovie )
		SetMovieVolume( maMovie, kFullVolume * (short)nVolumeDB / 100 );
}

// ----------------------------------------------------------------------------
	
sal_Int16 SAL_CALL Player::getVolumeDB() throw( RuntimeException )
{
	sal_Int16 nRet = 0;

	if ( maMovie )
		nRet = 100 * GetMovieVolume( maMovie ) / kFullVolume;

	return nRet;
}

// ----------------------------------------------------------------------------

Size SAL_CALL Player::getPreferredPlayerWindowSize() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::getPreferredPlayerWindowSize not implemented\n" );
#endif
	Size aSize( 0, 0 );
	return aSize;
}

// ----------------------------------------------------------------------------

Reference< XPlayerWindow > SAL_CALL Player::createPlayerWindow( const Sequence< Any >& aArguments ) throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::createPlayerWindow not implemented\n" );
#endif
	Reference< XPlayerWindow > xRet;
	return xRet;
}

// ----------------------------------------------------------------------------

Reference< XFrameGrabber > SAL_CALL Player::createFrameGrabber() throw( RuntimeException )
{
#ifdef DEBUG
	fprintf( stderr, "Player::createFrameGrabber not implemented\n" );
#endif
	Reference< XFrameGrabber > xRet;
	return xRet;
}

// ----------------------------------------------------------------------------

::rtl::OUString SAL_CALL Player::getImplementationName() throw( RuntimeException )
{
	return ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( AVMEDIA_QUICKTIME_PLAYER_IMPLEMENTATIONNAME ) );
}

// ----------------------------------------------------------------------------

sal_Bool SAL_CALL Player::supportsService( const ::rtl::OUString& ServiceName ) throw( RuntimeException )
{
	return ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( AVMEDIA_QUICKTIME_PLAYER_SERVICENAME ) );
}

// ----------------------------------------------------------------------------

Sequence< ::rtl::OUString > SAL_CALL Player::getSupportedServiceNames() throw( RuntimeException )
{
	Sequence< ::rtl::OUString > aRet(1);
	aRet[0] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM ( AVMEDIA_QUICKTIME_PLAYER_SERVICENAME ) );

	return aRet;
}

}	// namespace quicktime
}	// namespace avmedia
