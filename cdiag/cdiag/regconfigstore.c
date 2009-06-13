/*----------------------------------------------------------------------
 * Purpose:
 *		Implementation of the registry configuration store
 *
 * Copyright:
 *		2007, 2008 Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */

#define CDIAGAPI

#include "cdiagp.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )


/*++
	get array index from a _CDIAG_CONFIGURATION_SCOPES scope
--*/
#define KEY_INDEX_FOR_SCOPE( scope ) ( ( scope - 1 ) % 2 )
#define KEY_SCOPE_FOR_INDEX( scope ) ( ( scope + 1 ) )
#define MAX_KEY_INDEX 1

/*++
	limits - note that these limits are tighter than those
	imposed by the registry
--*/
#define MAX_REG_NAME_CCH 100
#define MAX_REG_VALUE_CB ( 1024 * 1024 ) 
#define MAX_REG_VALUE_CCH ( MAX_REG_VALUE_CB / sizeof( WCHAR ) ) 

typedef struct _CDIAGP_REG_CONFIG_STORE
{
	CDIAG_CONFIGURATION_STORE Base;

	//
	// Registry keys. Use KEY_INDEX_FOR_SCOPE to calculate index.
	//
	// Initialized on creation, read-only from then on.
	//
	HKEY Key[ 2 ];
	DWORD KeyAccessMask[ 2 ];

	struct _Notify
	{
		//
		// Lock that guards accesses to the Notify struct
		//
		CRITICAL_SECTION Lock;

		//
		// The event used by RegNotifyChangeKeyValue. Shared by
		// both scopes.
		//
		// Initialized lazily, read-only from then on.
		//

		HANDLE Event;
		
		//
		// Update callback wait handle. 
		// Use KEY_INDEX_FOR_SCOPE to calculate index.
		//
		// Initialized lazily, read-only from then on.
		//
		HANDLE WaitHandle;

		//
		// Update callback
		//
		// Changes at any time.
		//
		struct _Callback
		{
			CDIAG_CONFIGSTORE_UPDATE_CALLBACK Function;
			PVOID Context;
		} Callback;	
	} Notify;
} CDIAGP_REG_CONFIG_STORE, *PCDIAGP_REG_CONFIG_STORE;

/*----------------------------------------------------------------------
 *
 * Private helper routines
 *
 */

#define CdiagsIsValidScope( scope ) ( scope >= 1 && scope <= 3 )
#define CdiagsIsValidRegStore( store ) \
	( ( store != NULL && store->Base.Size == sizeof( CDIAGP_REG_CONFIG_STORE ) ) )

/*++
	Routine description:
		Tests whether a name is valid for use as a registry Key or
		value name.
--*/
static BOOL CdiagsIsValidRegistryName(
	__in LPCWSTR Name,
	__in size_t MaxLen
	)
{
	_ASSERTE( Name );
	_ASSERTE( MaxLen );

	return NULL != Name &&
		   NULL == wcschr( Name, L'\\' ) &&
		   CdiagpIsStringValid( Name, 1, MaxLen, FALSE );
}

/*++
	Routine description:
		Deletes a setting using a single scope, i.e. either HKCU or
		HKLM will be used, never both.
--*/
static HRESULT CdiagsDeleteSettingInSingleScope(
	__in PCDIAGP_REG_CONFIG_STORE Store,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope
	)
{
	LONG Res = 0;
	HKEY Key = NULL;
	HRESULT Hr = E_UNEXPECTED;

	_ASSERTE( Store );
	_ASSERTE( GroupName );
	_ASSERTE( Name );
	_ASSERTE( Scope == CdiagGlobalScope || Scope == CdiagUserScope );
	
	if ( Store->Key[ KEY_INDEX_FOR_SCOPE( Scope ) ] == NULL )
	{
		return E_ACCESSDENIED;
	}

	Res = RegOpenKeyEx(
		Store->Key[ KEY_INDEX_FOR_SCOPE( Scope ) ],
		GroupName,
		0,
		Store->KeyAccessMask[ KEY_INDEX_FOR_SCOPE( Scope ) ] & KEY_SET_VALUE,
		&Key );
	if ( ERROR_FILE_NOT_FOUND == Res )
	{
		return CDIAG_E_SETTING_NOT_FOUND;
	}
	else if ( ERROR_SUCCESS != Res )
	{
		return HRESULT_FROM_WIN32( Res );
	}

	Res = RegDeleteValue( Key, Name );
	if ( ERROR_FILE_NOT_FOUND == Res )
	{
		Hr = CDIAG_E_SETTING_NOT_FOUND;
	}
	else if ( ERROR_SUCCESS != Res )
	{
		Hr = HRESULT_FROM_WIN32( Res );
	}
	else
	{
		Hr = S_OK;
	}

	_VERIFY( ERROR_SUCCESS == RegCloseKey( Key ) );

	return Hr;
}

/*++
	Routine description:
		Writes a setting using a single scope, i.e. either HKCU or
		HKLM will be used, never both.
--*/
static HRESULT CdiagsWriteSettingInSingleScope(
	__in PCDIAGP_REG_CONFIG_STORE Store,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in DWORD DataType,
	__in DWORD DataLen,
	__in CONST PBYTE Data
	)
{
	LONG Res = 0;
	HKEY Key = NULL;
	HRESULT Hr = E_UNEXPECTED;

	_ASSERTE( Store );
	_ASSERTE( GroupName );
	_ASSERTE( Name );
	_ASSERTE( Scope == CdiagGlobalScope || Scope == CdiagUserScope );
	_ASSERTE( DataType );
	_ASSERTE( DataLen );
	_ASSERTE( Data );

	if ( Store->Key[ KEY_INDEX_FOR_SCOPE( Scope ) ] == NULL )
	{
		return E_ACCESSDENIED;
	}

	Res = RegCreateKeyEx(
		Store->Key[ KEY_INDEX_FOR_SCOPE( Scope ) ],
		GroupName,
		0,
		NULL,
		0,
		Store->KeyAccessMask[ KEY_INDEX_FOR_SCOPE( Scope ) ] & KEY_WRITE,
		NULL,
		&Key,
		NULL );
	if ( ERROR_SUCCESS != Res )
	{
		return HRESULT_FROM_WIN32( Res );
	}

	Res = RegSetValueEx(
		Key,
		Name,
		0,
		DataType,
		( PBYTE ) Data,
		DataLen );
	if ( ERROR_SUCCESS != Res )
	{
		Hr = HRESULT_FROM_WIN32( Res );
	}
	else
	{
		Hr = S_OK;
	}

	_VERIFY( ERROR_SUCCESS == RegCloseKey( Key ) );

	return Hr;
}

/*++
	Routine description:
		Reads a setting using a single scope, i.e. either HKCU or
		HKLM will be used, never both.
--*/
static HRESULT CdiagsReadSettingInSingleScope(
	__in PCDIAGP_REG_CONFIG_STORE Store,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in DWORD ExpectedDataType,
	__in DWORD BufferLen,
	__out PVOID Buffer,
	__out_opt DWORD *ActualLen
	)
{
	LONG Res = 0;
	DWORD DataType = 0;
	HKEY Key = NULL;
	HRESULT Hr = E_UNEXPECTED;
	DWORD DataRead = BufferLen;

	_ASSERTE( Store );
	_ASSERTE( GroupName );
	_ASSERTE( Name );
	_ASSERTE( Scope == CdiagGlobalScope || Scope == CdiagUserScope );
	_ASSERTE( ExpectedDataType );
	_ASSERTE( BufferLen );
	_ASSERTE( Buffer );

	if ( Store->Key[ KEY_INDEX_FOR_SCOPE( Scope ) ] == NULL )
	{
		return E_ACCESSDENIED;
	}

	Res = RegOpenKeyEx(
		Store->Key[ KEY_INDEX_FOR_SCOPE( Scope ) ],
		GroupName,
		0,
		Store->KeyAccessMask[ KEY_INDEX_FOR_SCOPE( Scope ) ] & KEY_READ,
		&Key );
	if ( ERROR_FILE_NOT_FOUND == Res )
	{
		return CDIAG_E_SETTING_NOT_FOUND;
	}
	else if ( ERROR_SUCCESS != Res )
	{
		return HRESULT_FROM_WIN32( Res );
	}

	Res = RegQueryValueEx(
		Key,
		Name,
		0,
		&DataType,
		Buffer,
		&DataRead );
	if ( ERROR_FILE_NOT_FOUND == Res )
	{
		Hr = CDIAG_E_SETTING_NOT_FOUND;
	}
	else if ( ERROR_MORE_DATA == Res )
	{
		if ( ActualLen )
		{
			*ActualLen = DataRead;
		}

		Hr = CDIAG_E_BUFFER_TOO_SMALL;
	}
	else if ( ERROR_SUCCESS != Res )
	{
		Hr = HRESULT_FROM_WIN32( Res );
	}
	else if ( DataType != ExpectedDataType )
	{
		Hr = CDIAG_E_SETTING_MISMATCH;
	}
	else
	{
		if ( ActualLen )
		{
			*ActualLen = DataRead;
		}
		Hr = S_OK;
	}

	_VERIFY( ERROR_SUCCESS == RegCloseKey( Key ) );

	return Hr;
}

/*++
	Routine description:
		Reads from the desired scope - possibly requiring reading
		both from HKLM and HKCU.
--*/
static HRESULT CdiagsReadSetting(
	__in PCDIAGP_REG_CONFIG_STORE Store,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in DWORD ExpectedDataType,
	__in DWORD BufferLen,
	__out PVOID Buffer,
	__out_opt DWORD *ActualLen
	)
{
	_ASSERTE( Store );
	_ASSERTE( GroupName );
	_ASSERTE( Name );
	_ASSERTE( CdiagsIsValidScope( Scope ) );
	_ASSERTE( ExpectedDataType );
	_ASSERTE( BufferLen );
	_ASSERTE( Buffer );

	if ( Scope & CdiagUserScope )
	{
		HRESULT Hr = CdiagsReadSettingInSingleScope(
			Store,
			GroupName,
			Name,
			CdiagUserScope,
			ExpectedDataType,
			BufferLen,
			Buffer,
			ActualLen );
		if ( SUCCEEDED( Hr ) )
		{
			//
			// User scope always wins, done
			//
			return S_OK;
		}
		else if ( CDIAG_E_SETTING_NOT_FOUND == Hr )
		{
			// 
			// Contine with global scope
			//
		}
		else
		{
			//
			// Any other failure, bail out
			//
			return Hr;
		}
	}
	
	if ( Scope & CdiagGlobalScope )
	{
		return CdiagsReadSettingInSingleScope(
			Store,
			GroupName,
			Name,
			CdiagGlobalScope,
			ExpectedDataType,
			BufferLen,
			Buffer,
			ActualLen );
	}

	return CDIAG_E_SETTING_NOT_FOUND;
}

/*----------------------------------------------------------------------
 *
 * Update notification callback handling helper routines
 *
 */

/*++
	Create the notification event and register a wait callback for it
--*/
static HRESULT CdiagsCreateNotificationEventAndRegisterCallback(
	__in PCDIAGP_REG_CONFIG_STORE Store,
	__in WAITORTIMERCALLBACK Callback,
	__in PVOID CallbackContext
	)
{
	HRESULT Hr = E_UNEXPECTED;

	_ASSERTE( Store );
	_ASSERTE( Callback );
	_ASSERTE( NULL == Store->Notify.Event );
	
	//
	// Note: event is manual-reset.
	//
	Store->Notify.Event = CreateEvent( NULL, TRUE, FALSE, NULL );
		
	if ( ! Store->Notify.Event )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	// 
	// Register a wait for the event. Use persisten thread as 
	// RegNotifyChangeKeyValue requires so.
	//
	if ( ! RegisterWaitForSingleObject(
		&Store->Notify.WaitHandle,
		Store->Notify.Event,
		Callback,
		CallbackContext,
		INFINITE,
		WT_EXECUTEINPERSISTENTTHREAD ) )
	{
		Hr = HRESULT_FROM_WIN32( GetLastError() );
		goto Cleanup;
	}

	Hr = S_OK;

Cleanup:
	if ( FAILED ( Hr ) )
	{
		if ( Store->Notify.WaitHandle )
		{
			_VERIFY( UnregisterWait( Store->Notify.WaitHandle ) );
			Store->Notify.WaitHandle = NULL;
		}

		if ( Store->Notify.Event )
		{
			_VERIFY( CloseHandle( Store->Notify.Event ) );
		}
	}

	return Hr;
}

/*++
	Register a callback for Key change notifications
--*/
static HRESULT CdiagsRegisterKeyChangeNotification(
	__in PCDIAGP_REG_CONFIG_STORE Store,
	__in HANDLE Key,
	__in WAITORTIMERCALLBACK Callback,
	__in PVOID CallbackContext
	)
{
	LONG Res;
	HRESULT Hr = E_UNEXPECTED;

	_ASSERTE( Store );
	_ASSERTE( Key );
	_ASSERTE( Callback );

	//
	// Has the event already been created?
	//
	if ( NULL != Store->Notify.Event && 
		 NULL != Store->Notify.WaitHandle )
	{
		//
		// Already set up
		//
		return S_OK;
	}
	
	//
	// If no event yet, create one.
	//
	if ( ! Store->Notify.Event )
	{
		Hr = CdiagsCreateNotificationEventAndRegisterCallback( 
			Store,
			Callback,
			CallbackContext );
		if ( FAILED( Hr ) )
		{
			return Hr;
		}
	}

	//
	// Register for Key change notifications on the given Key
	//
	Res = RegNotifyChangeKeyValue( 
		Key,
		TRUE,
		REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
		Store->Notify.Event,
		TRUE );
	if ( ERROR_SUCCESS != Res )
	{
		Hr = HRESULT_FROM_WIN32( Res );
		return Hr;
	}

	return S_OK;
}

/*++
	The callback function registered for the event
--*/
static VOID CdiagsKeyChangeNotificationCallback(
	__in PVOID Context,
	__in BOOLEAN TimedOut
	)
{
	UINT i;
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) Context;
	_ASSERTE( CdiagsIsValidRegStore( Store ) );
	
	UNREFERENCED_PARAMETER( TimedOut );

	//
	// Grab CS to avoid the callback function being changed - as soon as 
	// the callback function is changed, the context may not be valid any
	// more.
	//
	EnterCriticalSection( &Store->Notify.Lock );
	if ( Store->Notify.Callback.Function )
	{
		Store->Notify.Callback.Function( Store->Notify.Callback.Context );
	}
	LeaveCriticalSection( &Store->Notify.Lock );

	//
	// Register for next change event.
	//
	for ( i = 0; i <= MAX_KEY_INDEX; i++ )
	{
		if ( Store->Key[ i ] )
		{
			_VERIFY( ERROR_SUCCESS == RegNotifyChangeKeyValue( 
				Store->Key[ i ],
				TRUE,
				REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET,
				Store->Notify.Event,
				TRUE ) );
		}
	}
}

/*----------------------------------------------------------------------
 *
 * CDIAG_CONFIGURATION_STORE methods
 *
 */

static HRESULT CdiagsReadDwordSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__out DWORD *Value
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;
	DWORD Data = 0;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ! CdiagsIsValidScope( Scope ) ||
		 ! Value )
	{
		return E_INVALIDARG;
	}

	*Value = 0;

	Hr = CdiagsReadSetting(
		Store,
		GroupName,
		Name,
		Scope,
		REG_DWORD,
		sizeof( DWORD ),
		&Data,
		NULL );
	if ( CDIAG_E_BUFFER_TOO_SMALL == Hr )
	{
		//
		// A wrong buffer size can only be due to a mismatched Data type
		//
		return CDIAG_E_SETTING_MISMATCH;
	}
	else if ( FAILED( Hr ) )
	{
		return Hr;
	}
	else
	{
		*Value = Data;
		return S_OK;
	}
}

static HRESULT CdiagsReadStringSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in SIZE_T BufferSizeInChars,
	__out_ecount( BufferSize ) PWSTR StringBuffer,
	__out_opt DWORD *ActualSize
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ! CdiagsIsValidScope( Scope ) ||
		 BufferSizeInChars == 0 || BufferSizeInChars > MAX_REG_VALUE_CCH ||
		 ! StringBuffer )
	{
		return E_INVALIDARG;
	}

	return CdiagsReadSetting(
		Store,
		GroupName,
		Name,
		Scope,
		REG_SZ,
		( DWORD ) ( BufferSizeInChars * sizeof( WCHAR ) ),
		StringBuffer,
		ActualSize );
}

static HRESULT CdiagsReadMultiStringSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in SIZE_T BufferSizeInChars,
	__out_ecount( BufferSize ) PWSTR StringBuffer,
	__out_opt DWORD *ActualSize
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ! CdiagsIsValidScope( Scope ) ||
		 BufferSizeInChars == 0 || BufferSizeInChars > MAX_REG_VALUE_CCH ||
		 ! StringBuffer )
	{
		return E_INVALIDARG;
	}

	return CdiagsReadSetting(
		Store,
		GroupName,
		Name,
		Scope,
		REG_MULTI_SZ,
		( DWORD ) ( BufferSizeInChars * sizeof( WCHAR ) ),
		StringBuffer,
		ActualSize );
}

static HRESULT CdiagsWriteDwordSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in DWORD Value
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ( Scope != CdiagUserScope && Scope != CdiagGlobalScope ) )
	{
		return E_INVALIDARG;
	}

	return CdiagsWriteSettingInSingleScope(
		Store,
		GroupName,
		Name,
		Scope,
		REG_DWORD,
		sizeof( DWORD ),
		( PBYTE ) &Value );
}

static HRESULT CdiagsWriteStringSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in PCWSTR Value
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;
	HRESULT Hr = E_UNEXPECTED;
	size_t stringLenCch = 0;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ( Scope != CdiagUserScope && Scope != CdiagGlobalScope ) ||
		 ! Value )
	{
		return E_INVALIDARG;
	}

	Hr = StringCchLength( Value, MAX_REG_VALUE_CCH, &stringLenCch );
	if ( FAILED( Hr ) )
	{
		return E_INVALIDARG;
	}

	return CdiagsWriteSettingInSingleScope(
		Store,
		GroupName,
		Name,
		Scope,
		REG_SZ,
		( DWORD ) ( ( stringLenCch + 1 ) * sizeof( WCHAR ) ),
		( PBYTE ) Value );
}

static HRESULT CdiagsWriteMultiStringSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope,
	__in PCWSTR Value
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;
	HRESULT Hr = E_UNEXPECTED;
	size_t ElementLenCch = 0;
	size_t TotalLenCch = 0;
	PCWSTR Substr = Value;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ( Scope != CdiagUserScope && Scope != CdiagGlobalScope ) ||
		 ! Value )
	{
		return E_INVALIDARG;
	}

	do
	{
		Hr = StringCchLength( Substr, MAX_REG_VALUE_CCH, &ElementLenCch );
		if ( FAILED( Hr ) )
		{
			return E_INVALIDARG;
		}

		Substr += ElementLenCch + 1;
		TotalLenCch += ElementLenCch + 1;
	} 
	while ( SUCCEEDED( Hr ) && ElementLenCch > 0 );

	return CdiagsWriteSettingInSingleScope(
		Store,
		GroupName,
		Name,
		Scope,
		REG_MULTI_SZ,
		( DWORD ) ( TotalLenCch * sizeof( WCHAR ) ),
		( PBYTE ) Value );
}

static HRESULT CdiagsDeleteSetting(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in PCWSTR GroupName,
	__in PCWSTR Name,
	__in DWORD Scope
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ! CdiagsIsValidRegistryName( GroupName, MAX_SETTINGGROUPNAME_CCH ) ||
		 ! CdiagsIsValidRegistryName( Name, MAX_SETTINGNAME_CCH ) ||
		 ( Scope != CdiagUserScope && Scope != CdiagGlobalScope ) )
	{
		return E_INVALIDARG;
	}

	return CdiagsDeleteSettingInSingleScope(
		Store,
		GroupName,
		Name,
		Scope );
}

static HRESULT CdiagsRegisterUpdateCallback(
	__in PCDIAG_CONFIGURATION_STORE This,
	__in_opt CDIAG_CONFIGSTORE_UPDATE_CALLBACK UpdateCallback,
	__in_opt PVOID UpdateCallbackContext
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;
	HRESULT Hr = E_UNEXPECTED;

	if ( ! CdiagsIsValidRegStore( Store ) ||
		 ( UpdateCallbackContext != NULL && UpdateCallback == NULL ) )
	{
		return E_INVALIDARG;
	}

	_ASSERTE( Store->Key[ 0 ] != NULL || Store->Key[ 1 ] != NULL );

	//
	// We are going to change the Notify struct, so lock it
	//
	EnterCriticalSection( &Store->Notify.Lock );

	if ( UpdateCallback )
	{
		//
		// Register callback.
		//
		// If this is the first time, setup events and notification.
		//
		UINT i;
		for ( i = 0; i <= MAX_KEY_INDEX; i++ )
		{
			//
			// Register for all opened keys
			//
			if ( Store->Key[ i ] )
			{
				Hr = CdiagsRegisterKeyChangeNotification(
					Store,
					Store->Key[ i ],
					CdiagsKeyChangeNotificationCallback,
					Store );
				if ( FAILED ( Hr ) )
				{
					goto Cleanup;
				}
			}
		}

		//
		// Events and registration are in place, set the callback
		//
		Store->Notify.Callback.Function = UpdateCallback;
		Store->Notify.Callback.Context = UpdateCallbackContext;
	}
	else
	{
		//
		// Unregister callback. We cannot really unregister
		// for registry notifications until the Key is closed, as
		// there is no counterpart to RegNotifyChangeKeyValue.
		//
		Store->Notify.Callback.Function = NULL;
		Store->Notify.Callback.Context = NULL;
	}

	Hr = S_OK;

Cleanup:
	LeaveCriticalSection( &Store->Notify.Lock );

	//
	// ready for further notifications
	//
	ResetEvent( Store->Notify.Event );

	return Hr;
}


static HRESULT CdiagsDeleteStore(
	__in PCDIAG_CONFIGURATION_STORE This
	)
{
	PCDIAGP_REG_CONFIG_STORE Store = ( PCDIAGP_REG_CONFIG_STORE ) This;
	if ( CdiagsIsValidRegStore( Store ) )
	{
		UINT i;

		if ( Store->Notify.WaitHandle )
		{
			//
			// Stop waiting for events.
			// Note that UnregisterWaitEx is used to wait until all
			// callbacks have been completed - otherwise our callback
			// might touch freed memory.
			//
			if ( ! UnregisterWaitEx( 
				Store->Notify.WaitHandle, 
				INVALID_HANDLE_VALUE ) )
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}
		}
		
		for ( i = 0; i <= MAX_KEY_INDEX; i++ )
		{
			
			if ( Store->Key[ i ] )
			{
				if ( ! CloseHandle( Store->Key[ i ] ) )
				{
					return HRESULT_FROM_WIN32( GetLastError() );
				}
			}
		}

		if ( Store->Notify.Event )
		{
			if( ! CloseHandle( Store->Notify.Event ) )
			{
				return HRESULT_FROM_WIN32( GetLastError() );
			}
		}

		DeleteCriticalSection( &Store->Notify.Lock );
	}

	//
	// We only free if all steps above were successful - otherwise
	// a memory corruption might occur.
	//
	CdiagpFree( Store );

	return S_OK;
}

/*----------------------------------------------------------------------
 *
 * Public
 *
 */
HRESULT CDIAGCALLTYPE CdiagCreateRegistryStore(
	__in PCWSTR BaseKeyName,
	__in DWORD AccessMode,
	__out PCDIAG_CONFIGURATION_STORE *Store
	)
{
	HRESULT Hr = E_UNEXPECTED;
	PCDIAGP_REG_CONFIG_STORE TempStore = NULL;
	UINT i;
	LONG Result;

	if ( ! CdiagpIsStringValid( BaseKeyName, 1, MAX_REG_NAME_CCH, FALSE ) || 
		 ! Store || 
		 0 == AccessMode ||
		 AccessMode > CDIAG_CFGS_ACCESS_ALL )
	{
		Hr = E_INVALIDARG;
		goto Cleanup;
	}

	*Store = NULL;

	//
	// Allocate memory to hold structure
	//
	TempStore = CdiagpMalloc( 
		sizeof( CDIAGP_REG_CONFIG_STORE ),
		TRUE );
	if ( ! TempStore )
	{
		Hr = E_OUTOFMEMORY;
		goto Cleanup;
	}

	//
	// Try to open keys for machine and user
	//
	for ( i = 0; i <= MAX_KEY_INDEX; i++ )
	{
		HKEY Key;
		if ( i == KEY_INDEX_FOR_SCOPE( CdiagUserScope ) )
		{
			Key = HKEY_CURRENT_USER;
			TempStore->KeyAccessMask[ i ] =
				( AccessMode & CDIAG_CFGS_ACCESS_READ_USER ? KEY_READ : 0 ) |
				( AccessMode & CDIAG_CFGS_ACCESS_WRITE_USER ? KEY_WRITE | KEY_NOTIFY : 0 );
		}
		else
		{
			Key = HKEY_LOCAL_MACHINE;
			TempStore->KeyAccessMask[ i ] =
				( AccessMode & CDIAG_CFGS_ACCESS_READ_MACHINE ? KEY_READ : 0 ) |
				( AccessMode & CDIAG_CFGS_ACCESS_WRITE_MACHINE ? KEY_WRITE | KEY_NOTIFY : 0 );
		}
		
		if ( TempStore->KeyAccessMask[ i ] == 0 )
		{
			//
			// No need to open Key
			//
			TempStore->Key[ i ] = NULL;
		}
		else
		{
			Result = RegCreateKeyEx(
				Key,
				BaseKeyName,
				0,
				NULL,
				0,
				TempStore->KeyAccessMask[ i ],
				NULL,
				&TempStore->Key[ i ],
				NULL );
			if ( ERROR_SUCCESS != Result )
			{
				Hr = HRESULT_FROM_WIN32( Result );
				goto Cleanup;
			}
			_ASSERTE( TempStore->Key[ i ] );
		}
	}

	//
	// Initialize
	//
	TempStore->Base.Delete					= CdiagsDeleteStore;
	TempStore->Base.DeleteSetting			= CdiagsDeleteSetting;
	TempStore->Base.ReadDwordSetting		= CdiagsReadDwordSetting;
	TempStore->Base.ReadStringSetting		= CdiagsReadStringSetting;
	TempStore->Base.ReadMultiStringSetting	= CdiagsReadMultiStringSetting;
	TempStore->Base.WriteDwordSetting		= CdiagsWriteDwordSetting;
	TempStore->Base.RegisterUpdateCallback	= CdiagsRegisterUpdateCallback;
	TempStore->Base.WriteStringSetting		= CdiagsWriteStringSetting;
	TempStore->Base.WriteMultiStringSetting	= CdiagsWriteMultiStringSetting;

	TempStore->Base.Size = sizeof( CDIAGP_REG_CONFIG_STORE );

	InitializeCriticalSection( &TempStore->Notify.Lock );

	*Store = ( PCDIAG_CONFIGURATION_STORE ) TempStore;
	Hr = S_OK;

Cleanup:
	if ( FAILED( Hr ) )
	{
		if ( TempStore )
		{
			CdiagsDeleteStore( ( PCDIAG_CONFIGURATION_STORE ) TempStore );
		}
	}

	return Hr;
}
