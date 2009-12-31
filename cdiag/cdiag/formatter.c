/*----------------------------------------------------------------------
 * Purpose:
 *		Standard formatter implementation.
 *
 * Copyright:
 *		2007-2009 Johannes Passing (passing at users.sourceforge.net)
 *
 * This file is part of cfix.
 *
 * cfix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cfix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with cfix.  If not, see <http://www.gnu.org/licenses/>.
 */

#define CDIAGAPI

#include <stdlib.h>
#include "cdiagp.h"
#include "resource.h"

#pragma warning( push )
#pragma warning( disable: 6011; disable: 6387 )
#include <strsafe.h>
#pragma warning( pop )

/*----------------------------------------------------------------------
 *
 * Formatter object.
 *
 */
typedef struct _CDIAGP_FORMATTER
{
	CDIAG_FORMATTER Base;

	volatile LONG ReferenceCount;


	//
	// Template specifying the format - see CdiagpFormatString
	// for syntax.
	//
	PCWSTR FormatTemplate;

	//
	// Optional: Resolver object. 
	//
	PCDIAG_MESSAGE_RESOLVER Resolver;
	DWORD ResolvingFlags;
} CDIAGP_FORMATTER, *PCDIAGP_FORMATTER;

#define CdiagsIsValidFormatter( fmt ) \
	( ( fmt != NULL && fmt->Base.Size == sizeof( CDIAGP_FORMATTER ) ) )


/*----------------------------------------------------------------------
 *
 * Captions for enums.
 *
 */

#define IDS_FORMATTER_TYPE_BASE IDS_FORMATTER_TYPE_LOG
#define IDS_FORMATTER_SEV_BASE	IDS_FORMATTER_SEV_TRACE
#define IDS_FORMATTER_PROC_BASE	IDS_FORMATTER_PROC_KERNEL
#define MAX_CAPTION_CCH			64

static WCHAR CdiagsEventTypeCaptions[ CdiagMaxEvent + 1 ][ MAX_CAPTION_CCH ];
static WCHAR CdiagsSeverityCaptions[ CdiagMaxSeverity + 1 ][ MAX_CAPTION_CCH ];
static WCHAR CdiagsProcModeCaptions[ CdiagMaxMode + 1 ][ MAX_CAPTION_CCH ];

/*++
	Routine Description:
		Preload resources for fast access.

		Must be called from DllMain on process attachment.
--*/
VOID CdiagpInitializeFormatter()
{
	UINT Index;

	//
	// Load event type captions.
	//
	for ( Index = 0; Index <= CdiagMaxEvent; Index++ )
	{
		if ( 0 == LoadString(
			CdiagpModule,
			IDS_FORMATTER_TYPE_BASE + Index,
			CdiagsEventTypeCaptions[ Index ],
			MAX_CAPTION_CCH ) )
		{
			_ASSERTE( !"String resource missing" );
			StringCchPrintf(
				CdiagsEventTypeCaptions[ Index ],
				MAX_CAPTION_CCH,
				L"Type %d",
				Index );
		}
	}

	//
	// Load severity captions.
	//
	for ( Index = 0; Index <= CdiagMaxSeverity; Index++ )
	{
		if ( 0 == LoadString(
			CdiagpModule,
			IDS_FORMATTER_SEV_BASE + Index,
			CdiagsSeverityCaptions[ Index ],
			MAX_CAPTION_CCH ) )
		{
			_ASSERTE( !"String resource missing" );
			StringCchPrintf(
				CdiagsSeverityCaptions[ Index ],
				MAX_CAPTION_CCH,
				L"Severity %d",
				Index );
		}
	}

	//
	// Load processor mode captions.
	//
	for ( Index = 0; Index <= CdiagMaxMode; Index++ )
	{
		if ( 0 == LoadString(
			CdiagpModule,
			IDS_FORMATTER_PROC_BASE + Index,
			CdiagsProcModeCaptions[ Index ],
			MAX_CAPTION_CCH ) )
		{
			_ASSERTE( !"String resource missing" );
			StringCchPrintf(
				CdiagsSeverityCaptions[ Index ],
				MAX_CAPTION_CCH,
				L"Mode %d",
				Index );
		}
	}
}

/*----------------------------------------------------------------------
 *
 * Binding Helpers.
 *
 */
typedef VOID ( * TRANSLATE_ROUTINE )(
	__in PVOID RawData,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	);

typedef enum _BINDING_FIELD_TYPE
{
	PwstrType,
	WordType,
	DwordType,
	SpecialType		// requires translation routine
} BINDING_FIELD_TYPE;

typedef struct _BINDING_DEFINITION
{
	//
	// Offset of field within struct.
	//
	DWORD StructOffset;

	//
	// Data type of ultimate field.
	//
	BINDING_FIELD_TYPE Type;

	//
	// Does StructOffset point to the value (FALSE) or
	// specifies an offset to the value (TRUE)?
	//
	// If FALSE, StructOffset must refer to a 32 bit field.
	//
	BOOL IsOffset;

	//
	// Offset of structure referred to by field within struct.
	//
	DWORD OffsetWithinStruct;

	//
	// Is the field again an offset?
	//
	BOOL OffsetWithinStructIsOffset;

	//
	// Optional translation routine, e.g. to convert
	// from enum value to caption.
	//
	TRANSLATE_ROUTINE TranslateRoutine;
} BINDING_DEFINITION, *PBINDING_DEFINITION;

static VOID CdiagsTranslateEventType(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	DWORD Type = * ( PDWORD ) ValuePtr;
	if ( Type <= CdiagMaxEvent )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) CdiagsEventTypeCaptions[ Type ];
	}
	else
	{
		*Translated = 0;
	}
	*MustBeFreed = FALSE;
}

static VOID CdiagsTranslateSeverity(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	UCHAR Sev = * ( PUCHAR ) ValuePtr;
	if ( Sev <= CdiagMaxSeverity )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) CdiagsSeverityCaptions[ Sev ];
	}
	else
	{
		*Translated = 0;
	}
	*MustBeFreed = FALSE;
}

static VOID CdiagsTranslateProcessorMode(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	UCHAR Mode = * ( PUCHAR ) ValuePtr;
	if ( Mode <= CdiagMaxMode )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) CdiagsProcModeCaptions[ Mode];
	}
	else
	{
		*Translated = 0;
	}
	*MustBeFreed = FALSE;
}

static VOID CdiagsTranslateTimestamp(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	PFILETIME UtcTime = ( PFILETIME ) ValuePtr;
	FILETIME LocalTime;
	SYSTEMTIME LocalSystemTime;
	PWSTR Buffer = NULL;

	if ( ! FileTimeToLocalFileTime( UtcTime, &LocalTime ) )
	{
		*Translated = 0;
		*MustBeFreed = FALSE;
	}

	if ( ! FileTimeToSystemTime( &LocalTime, &LocalSystemTime ) )
	{
		*Translated = 0;
		*MustBeFreed = FALSE;
	}

	//
	// Allocate memory to hold formatted string.
	//
	Buffer = CdiagpMalloc( 100, TRUE );
	if ( ! Buffer )
	{
		*Translated = 0;
		*MustBeFreed = FALSE;
	}
	
	if ( GetTimeFormat(
		LOCALE_USER_DEFAULT,
		0,
		&LocalSystemTime,
		NULL,
		Buffer,
		100 ) )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) Buffer;
		*MustBeFreed = TRUE;
	}
	else
	{
		if ( Buffer )
		{
			CdiagpFree( Buffer );
		}

		*Translated = 0;
		*MustBeFreed = FALSE;
	}
}

/*----------------------------------------------------------------------
 *
 * Variable definition.
 *
 */
static FORMAT_VARIABLE CdiagsEventPacketVariables[] =
{
	{ L"Message",		L"%s" },
	{ L"Type",			L"%s" },
	{ L"Flags",			L"0x%04X" },
	{ L"Severity",		L"%s" },
	{ L"ProcessorMode", L"%s" },
	{ L"Machine",		L"%s" },
	{ L"ProcessId",		L"%u" },
	{ L"ThreadId",		L"%u" },
	{ L"Timestamp",		L"%s" },
	{ L"Code",			L"0x%08X" },
	{ L"Module",		L"%s" },
	{ L"Function",		L"%s" },
	{ L"File",			L"%s" },
	{ L"Line",			L"%u" },
};

static BINDING_DEFINITION CdiagsEventPacketBindings[] = 
{
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, MessageOffset ), PwstrType,	TRUE,	0, FALSE, NULL	},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, Type ),		  SpecialType,	FALSE,	0, FALSE, CdiagsTranslateEventType	},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, Flags ),		  WordType,		FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, Severity ),	  SpecialType,	FALSE,	0, FALSE, CdiagsTranslateSeverity	},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, ProcessorMode ), SpecialType,	FALSE,	0, FALSE, CdiagsTranslateProcessorMode },
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, MachineOffset ), PwstrType,	TRUE,	0, FALSE, NULL},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, ProcessId ),	  DwordType,	FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, ThreadId ),	  DwordType,	FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, Timestamp ),	  SpecialType,	FALSE,	0, FALSE, CdiagsTranslateTimestamp },
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, Code ),		  DwordType,	FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, DebugInfoOffset ), PwstrType,	TRUE,	FIELD_OFFSET( CDIAG_DEBUG_INFO, ModuleOffset ),		TRUE,  NULL},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, DebugInfoOffset ), PwstrType,	TRUE,	FIELD_OFFSET( CDIAG_DEBUG_INFO, FunctionNameOffset ),	TRUE,  NULL},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, DebugInfoOffset ), PwstrType,	TRUE,	FIELD_OFFSET( CDIAG_DEBUG_INFO, SourceFileOffset ),	TRUE,  NULL},
	{ FIELD_OFFSET( CDIAG_EVENT_PACKET, DebugInfoOffset ), DwordType,	TRUE,	FIELD_OFFSET( CDIAG_DEBUG_INFO, SourceLine ),			FALSE, NULL },
};

C_ASSERT( _countof( CdiagsEventPacketVariables ) == _countof( CdiagsEventPacketBindings ) );

/*----------------------------------------------------------------------
 *
 * Formatter implementation.
 *
 */

#define CdiagsGetFieldViaOffset( Struct, Offset ) \
	( ( PVOID ) ( ( PBYTE ) ( PVOID ) Struct + Offset ) )

static HRESULT CdiagsFormat(
	__in PCDIAG_FORMATTER This,
	__in CONST PCDIAG_EVENT_PACKET EventPkt,
	__in SIZE_T BufferSizeInChars,
	__out PWSTR Buffer
	)
{
	PCDIAGP_FORMATTER Formatter = ( PCDIAGP_FORMATTER ) This;
	DWORD_PTR Bindings[ _countof( CdiagsEventPacketVariables ) ];
	BOOL MustFree[ _countof( CdiagsEventPacketVariables ) ];
	UINT Index;
	HRESULT Hr;
	WCHAR ResolverBuffer[ 1024 ];
	PWSTR InsertionStrings[ CDIAG_EVENT_PACKET_MAX_INSERTION_STRINGS ];

	if ( ! CdiagsIsValidFormatter( Formatter ) ||
		 ! CdiagsIsValidEventPacket( EventPkt ) ||
		 BufferSizeInChars == 0 ||
		 ! Buffer )
	{
		return E_INVALIDARG;
	}

	//
	// Prepare bindings.
	//
	for ( Index = 0; Index <  _countof( CdiagsEventPacketVariables ); Index++ )
	{
		PVOID FieldPtr = CdiagsGetFieldViaOffset(
			EventPkt,
			CdiagsEventPacketBindings[ Index ].StructOffset );

		//
		// FieldPtr points to a field within struct. This field is 
		// either a direct value or a offset. Note that the size
		// of the field may vary.
		//
		if ( CdiagsEventPacketBindings[ Index ].IsOffset )
		{
			if ( * ( PDWORD ) FieldPtr != 0 )
			{
				//
				// Field is an offset -> field should be dereferenced. Offset
				// fields are always of type DWORD.
				//

				FieldPtr = CdiagsGetFieldViaOffset( 
						EventPkt,
						* ( PDWORD ) FieldPtr );
				
				//
				// FieldPtr is now either a direct value or again an offset.
				//
				if ( CdiagsEventPacketBindings[ Index ].OffsetWithinStruct != 0 )
				{
					if ( ( PDWORD ) FieldPtr != 0 )
					{
						PVOID StructPtr = FieldPtr;
						//
						// OffsetWithinStruct was set, so FieldPtr now
						// points to a struct, from which a field is to be 
						// retrieved.
						//
						// Get Ptr to field within struct.
						//
						FieldPtr = 
							CdiagsGetFieldViaOffset(
								StructPtr,
								CdiagsEventPacketBindings[ Index ].OffsetWithinStruct );

						if ( CdiagsEventPacketBindings[ Index ].OffsetWithinStructIsOffset )
						{
							if ( * ( PDWORD ) FieldPtr != 0 )
							{
								//
								// Field denotes an offset into StructPtr.
								//
								FieldPtr = CdiagsGetFieldViaOffset(
										StructPtr,
										* ( PDWORD ) FieldPtr );
							}
							else
							{
								FieldPtr = 0;
							}
						}
					}
					else
					{
						FieldPtr = 0;
					}
				}
			}
			else
			{
				FieldPtr = 0;
			}
		}
		else
		{
			//
			// Direct value.
			//
		}

		//
		// Translate raw value if neccessary.
		//
		MustFree[ Index ] = FALSE;
		if ( CdiagsEventPacketBindings[ Index ].TranslateRoutine )
		{
			_ASSERTE( CdiagsEventPacketBindings[ Index ].Type == SpecialType );
			CdiagsEventPacketBindings[ Index ].TranslateRoutine(
				FieldPtr,
				&Bindings[ Index ],
				&MustFree[ Index ] );
		}
		else
		{
			if ( FieldPtr == 0 )
			{
				Bindings[ Index ] = 0;
			}
			else
			{
				switch ( CdiagsEventPacketBindings[ Index ].Type )
				{
				case DwordType:
					Bindings[ Index ] = * ( PDWORD ) FieldPtr;
					break;

				case WordType:
					Bindings[ Index ] = * ( PWORD ) FieldPtr;
					break;

				case PwstrType:
					Bindings[ Index ] = ( DWORD_PTR ) ( PWSTR ) FieldPtr;
					break;

				default:
					_ASSERTE( !"Invalid data type" );
				}
			}
		}
	}

	//
	// Resolve message if neccessary.
	//
	if ( 0 == EventPkt->MessageOffset && Formatter->Resolver != NULL )
	{
		for ( Index = 0; Index < EventPkt->MessageInsertionStrings.Count; Index++ )
		{
			InsertionStrings[ Index ] = ( PWSTR )
				( ( PBYTE ) EventPkt + 
				  EventPkt->MessageInsertionStrings.Offset[ Index ] );
		}

		if ( SUCCEEDED( Formatter->Resolver->ResolveMessage(
			Formatter->Resolver,
			EventPkt->Code,
			Formatter->ResolvingFlags,
			EventPkt->MessageInsertionStrings.Count > 0
				? InsertionStrings
				: NULL,
			_countof( ResolverBuffer ),
			ResolverBuffer ) ) )
		{
			//
			// Update binding.
			//
			_ASSERTE( 0 == wcscmp( CdiagsEventPacketVariables[ 0 ].Name, L"Message" ) );
			Bindings[ 0 ] = ( DWORD_PTR ) ResolverBuffer;
		}
	}


	//
	// Format the message.
	//
	Hr = CdiagpFormatString(
		_countof( CdiagsEventPacketVariables ),
		CdiagsEventPacketVariables,
		Bindings,
		Formatter->FormatTemplate,
		BufferSizeInChars,
		Buffer );

	//
	// Free translation buffers.
	//
	for ( Index = 0; Index <  _countof( CdiagsEventPacketVariables ); Index++ )
	{
		if ( MustFree[ Index ] )
		{
			CdiagpFree( ( PVOID ) Bindings[ Index ] );
		}
	}

	return Hr;
}

static HRESULT CdiagsDeleteFormatter(
	__in PCDIAGP_FORMATTER Formatter
	)
{
	_ASSERTE( CdiagsIsValidFormatter( Formatter ) );
	
	if ( Formatter->Resolver )
	{
		Formatter->Resolver->Dereference( Formatter->Resolver );
	}

	CdiagpFree( Formatter );

	return S_OK;
}

static VOID CdiagsReferenceFormatter(
	__in PCDIAG_FORMATTER This
	)
{
	PCDIAGP_FORMATTER Formatter = ( PCDIAGP_FORMATTER ) This;
	_ASSERTE( CdiagsIsValidFormatter( Formatter ) );

	InterlockedIncrement( &Formatter->ReferenceCount );
}

static VOID CdiagsDereferenceFormatter(
	__in PCDIAG_FORMATTER This
	)
{
	PCDIAGP_FORMATTER Formatter = ( PCDIAGP_FORMATTER ) This;
	
	_ASSERTE( CdiagsIsValidFormatter( Formatter ) );

	if ( 0 == InterlockedDecrement( &Formatter->ReferenceCount ) )
	{
		_VERIFY( S_OK == CdiagsDeleteFormatter( Formatter ) );
	}
}

/*----------------------------------------------------------------------
 *
 * Public
 *
 */

HRESULT CDIAGCALLTYPE CdiagCreateFormatter(
	__in PCWSTR FormatTemplate,
	__in_opt PCDIAG_MESSAGE_RESOLVER Resolver,
	__in_opt DWORD ResolvingFlags,
	__out PCDIAG_FORMATTER *Result
	)
{
	PCDIAGP_FORMATTER Formatter = NULL;
	SIZE_T FormatTemplateCb;
	PWSTR Temp;

	if ( ! CdiagpIsStringValid( FormatTemplate, 1, MAXWORD, FALSE ) ||
		 ! Result ||
		 ( ResolvingFlags != 0 && ! Resolver ) )
	{
		return E_INVALIDARG;
	}

	FormatTemplateCb = ( wcslen( FormatTemplate ) + 1 ) * sizeof( WCHAR );

	//
	// Allocate space for object and format string.
	//
	Formatter = ( PCDIAGP_FORMATTER ) CdiagpMalloc( 
		sizeof( CDIAGP_FORMATTER ) + FormatTemplateCb, TRUE );
	if ( ! Formatter )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize it.
	//
	Temp = ( PWSTR ) 
		( ( PBYTE ) Formatter + sizeof( CDIAGP_FORMATTER ) );
	StringCbCopy(
		Temp,
		FormatTemplateCb,
		FormatTemplate );
	Formatter->ReferenceCount = 1;
	Formatter->FormatTemplate = Temp;

	Formatter->Base.Size			= sizeof( CDIAGP_FORMATTER );
	Formatter->Base.Dereference		= CdiagsDereferenceFormatter;
	Formatter->Base.Reference		= CdiagsReferenceFormatter;
	Formatter->Base.Format			= CdiagsFormat;
	if ( Resolver )
	{
		Resolver->Reference( Resolver );
		Formatter->Resolver = Resolver;
		Formatter->ResolvingFlags = ResolvingFlags;
	}

	*Result = &Formatter->Base;

	return S_OK;
}