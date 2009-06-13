/*----------------------------------------------------------------------
 * Purpose:
 *		Standard formatter implementation.
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

#define JPDIAGAPI

#include <stdlib.h>
#include "internal.h"
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
typedef struct _FORMATTER
{
	JPDIAG_FORMATTER Base;

	volatile LONG ReferenceCount;


	//
	// Template specifying the format - see JpdiagpFormatString
	// for syntax.
	//
	PCWSTR FormatTemplate;

	//
	// Optional: Resolver object. 
	//
	PJPDIAG_MESSAGE_RESOLVER Resolver;
	DWORD ResolvingFlags;
} FORMATTER, *PFORMATTER;

#define JpdiagsIsValidFormatter( fmt ) \
	( ( fmt != NULL && fmt->Base.Size == sizeof( FORMATTER ) ) )


/*----------------------------------------------------------------------
 *
 * Captions for enums.
 *
 */

#define IDS_FORMATTER_TYPE_BASE IDS_FORMATTER_TYPE_LOG
#define IDS_FORMATTER_SEV_BASE	IDS_FORMATTER_SEV_TRACE
#define IDS_FORMATTER_PROC_BASE	IDS_FORMATTER_PROC_KERNEL
#define MAX_CAPTION_CCH			64

static WCHAR JpdiagsEventTypeCaptions[ JpdiagMaxEvent + 1 ][ MAX_CAPTION_CCH ];
static WCHAR JpdiagsSeverityCaptions[ JpdiagMaxSeverity + 1 ][ MAX_CAPTION_CCH ];
static WCHAR JpdiagsProcModeCaptions[ JpdiagMaxMode + 1 ][ MAX_CAPTION_CCH ];

/*++
	Routine Description:
		Preload resources for fast access.

		Must be called from DllMain on process attachment.
--*/
VOID JpdiagpInitializeFormatter()
{
	UINT Index;

	//
	// Load event type captions.
	//
	for ( Index = 0; Index <= JpdiagMaxEvent; Index++ )
	{
		if ( 0 == LoadString(
			JpdiagpModule,
			IDS_FORMATTER_TYPE_BASE + Index,
			JpdiagsEventTypeCaptions[ Index ],
			MAX_CAPTION_CCH ) )
		{
			_ASSERTE( !"String resource missing" );
			StringCchPrintf(
				JpdiagsEventTypeCaptions[ Index ],
				MAX_CAPTION_CCH,
				L"Type %d",
				Index );
		}
	}

	//
	// Load severity captions.
	//
	for ( Index = 0; Index <= JpdiagMaxSeverity; Index++ )
	{
		if ( 0 == LoadString(
			JpdiagpModule,
			IDS_FORMATTER_SEV_BASE + Index,
			JpdiagsSeverityCaptions[ Index ],
			MAX_CAPTION_CCH ) )
		{
			_ASSERTE( !"String resource missing" );
			StringCchPrintf(
				JpdiagsSeverityCaptions[ Index ],
				MAX_CAPTION_CCH,
				L"Severity %d",
				Index );
		}
	}

	//
	// Load processor mode captions.
	//
	for ( Index = 0; Index <= JpdiagMaxMode; Index++ )
	{
		if ( 0 == LoadString(
			JpdiagpModule,
			IDS_FORMATTER_PROC_BASE + Index,
			JpdiagsProcModeCaptions[ Index ],
			MAX_CAPTION_CCH ) )
		{
			_ASSERTE( !"String resource missing" );
			StringCchPrintf(
				JpdiagsSeverityCaptions[ Index ],
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

static VOID JpdiagsTranslateEventType(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	DWORD Type = * ( PDWORD ) ValuePtr;
	if ( Type <= JpdiagMaxEvent )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) JpdiagsEventTypeCaptions[ Type ];
	}
	else
	{
		*Translated = 0;
	}
	*MustBeFreed = FALSE;
}

static VOID JpdiagsTranslateSeverity(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	UCHAR Sev = * ( PUCHAR ) ValuePtr;
	if ( Sev <= JpdiagMaxSeverity )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) JpdiagsSeverityCaptions[ Sev ];
	}
	else
	{
		*Translated = 0;
	}
	*MustBeFreed = FALSE;
}

static VOID JpdiagsTranslateProcessorMode(
	__in PVOID ValuePtr,
	__out DWORD_PTR *Translated,
	__out PBOOL MustBeFreed
	)
{
	UCHAR Mode = * ( PUCHAR ) ValuePtr;
	if ( Mode <= JpdiagMaxMode )
	{
		*Translated = ( DWORD_PTR ) ( PVOID ) JpdiagsProcModeCaptions[ Mode];
	}
	else
	{
		*Translated = 0;
	}
	*MustBeFreed = FALSE;
}

static VOID JpdiagsTranslateTimestamp(
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
	Buffer = JpdiagpMalloc( 100, TRUE );
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
		JpdiagpFree( Buffer );
		*Translated = 0;
		*MustBeFreed = FALSE;
	}
}

/*----------------------------------------------------------------------
 *
 * Variable definition.
 *
 */
static FORMAT_VARIABLE JpdiagsEventPacketVariables[] =
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

static BINDING_DEFINITION JpdiagsEventPacketBindings[] = 
{
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, MessageOffset ), PwstrType,	TRUE,	0, FALSE, NULL	},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, Type ),		  SpecialType,	FALSE,	0, FALSE, JpdiagsTranslateEventType	},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, Flags ),		  WordType,		FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, Severity ),	  SpecialType,	FALSE,	0, FALSE, JpdiagsTranslateSeverity	},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, ProcessorMode ), SpecialType,	FALSE,	0, FALSE, JpdiagsTranslateProcessorMode },
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, MachineOffset ), PwstrType,	TRUE,	0, FALSE, NULL},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, ProcessId ),	  DwordType,	FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, ThreadId ),	  DwordType,	FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, Timestamp ),	  SpecialType,	FALSE,	0, FALSE, JpdiagsTranslateTimestamp },
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, Code ),		  DwordType,	FALSE,	0, FALSE, NULL },
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, DebugInfoOffset ), PwstrType,	TRUE,	FIELD_OFFSET( JPDIAG_DEBUG_INFO, ModuleOffset ),		TRUE,  NULL},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, DebugInfoOffset ), PwstrType,	TRUE,	FIELD_OFFSET( JPDIAG_DEBUG_INFO, FunctionNameOffset ),	TRUE,  NULL},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, DebugInfoOffset ), PwstrType,	TRUE,	FIELD_OFFSET( JPDIAG_DEBUG_INFO, SourceFileOffset ),	TRUE,  NULL},
	{ FIELD_OFFSET( JPDIAG_EVENT_PACKET, DebugInfoOffset ), DwordType,	TRUE,	FIELD_OFFSET( JPDIAG_DEBUG_INFO, SourceLine ),			FALSE, NULL },
};

C_ASSERT( _countof( JpdiagsEventPacketVariables ) == _countof( JpdiagsEventPacketBindings ) );

/*----------------------------------------------------------------------
 *
 * Formatter implementation.
 *
 */

#define JpdiagsGetFieldViaOffset( Struct, Offset ) \
	( ( PVOID ) ( ( PBYTE ) ( PVOID ) Struct + Offset ) )

static HRESULT JpdiagsFormat(
	__in PJPDIAG_FORMATTER This,
	__in CONST PJPDIAG_EVENT_PACKET EventPkt,
	__in SIZE_T BufferSizeInChars,
	__out PWSTR Buffer
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	DWORD_PTR Bindings[ _countof( JpdiagsEventPacketVariables ) ];
	BOOL MustFree[ _countof( JpdiagsEventPacketVariables ) ];
	UINT Index;
	HRESULT Hr;
	WCHAR ResolverBuffer[ 1024 ];
	PWSTR InsertionStrings[ JPDIAG_EVENT_PACKET_MAX_INSERTION_STRINGS ];

	if ( ! JpdiagsIsValidFormatter( Formatter ) ||
		 ! JpdiagsIsValidEventPacket( EventPkt ) ||
		 BufferSizeInChars == 0 ||
		 ! Buffer )
	{
		return E_INVALIDARG;
	}

	//
	// Prepare bindings.
	//
	for ( Index = 0; Index <  _countof( JpdiagsEventPacketVariables ); Index++ )
	{
		PVOID FieldPtr = JpdiagsGetFieldViaOffset(
			EventPkt,
			JpdiagsEventPacketBindings[ Index ].StructOffset );

		//
		// FieldPtr points to a field within struct. This field is 
		// either a direct value or a offset. Note that the size
		// of the field may vary.
		//
		if ( JpdiagsEventPacketBindings[ Index ].IsOffset )
		{
			if ( * ( PDWORD ) FieldPtr != 0 )
			{
				//
				// Field is an offset -> field should be dereferenced. Offset
				// fields are always of type DWORD.
				//

				FieldPtr = JpdiagsGetFieldViaOffset( 
						EventPkt,
						* ( PDWORD ) FieldPtr );
				
				//
				// FieldPtr is now either a direct value or again an offset.
				//
				if ( JpdiagsEventPacketBindings[ Index ].OffsetWithinStruct != 0 )
				{
					if ( * ( PDWORD ) FieldPtr != 0 )
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
							JpdiagsGetFieldViaOffset(
								StructPtr,
								JpdiagsEventPacketBindings[ Index ].OffsetWithinStruct );

						if ( JpdiagsEventPacketBindings[ Index ].OffsetWithinStructIsOffset )
						{
							if ( * ( PDWORD ) FieldPtr != 0 )
							{
								//
								// Field denotes an offset into StructPtr.
								//
								FieldPtr = JpdiagsGetFieldViaOffset(
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
		if ( JpdiagsEventPacketBindings[ Index ].TranslateRoutine )
		{
			_ASSERTE( JpdiagsEventPacketBindings[ Index ].Type == SpecialType );
			JpdiagsEventPacketBindings[ Index ].TranslateRoutine(
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
				switch ( JpdiagsEventPacketBindings[ Index ].Type )
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
			_ASSERTE( 0 == wcscmp( JpdiagsEventPacketVariables[ 0 ].Name, L"Message" ) );
			Bindings[ 0 ] = ( DWORD_PTR ) ResolverBuffer;
		}
	}


	//
	// Format the message.
	//
	Hr = JpdiagpFormatString(
		_countof( JpdiagsEventPacketVariables ),
		JpdiagsEventPacketVariables,
		Bindings,
		Formatter->FormatTemplate,
		BufferSizeInChars,
		Buffer );

	//
	// Free translation buffers.
	//
	for ( Index = 0; Index <  _countof( JpdiagsEventPacketVariables ); Index++ )
	{
		if ( MustFree[ Index ] )
		{
			JpdiagpFree( ( PVOID ) Bindings[ Index ] );
		}
	}

	return Hr;
}

static HRESULT JpdiagsDeleteFormatter(
	__in PFORMATTER Formatter
	)
{
	_ASSERTE( JpdiagsIsValidFormatter( Formatter ) );
	
	if ( Formatter->Resolver )
	{
		Formatter->Resolver->Dereference( Formatter->Resolver );
	}

	JpdiagpFree( Formatter );

	return S_OK;
}

static VOID JpdiagsReferenceFormatter(
	__in PJPDIAG_FORMATTER This
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	_ASSERTE( JpdiagsIsValidFormatter( Formatter ) );

	InterlockedIncrement( &Formatter->ReferenceCount );
}

static VOID JpdiagsDereferenceFormatter(
	__in PJPDIAG_FORMATTER This
	)
{
	PFORMATTER Formatter = ( PFORMATTER ) This;
	
	_ASSERTE( JpdiagsIsValidFormatter( Formatter ) );

	if ( 0 == InterlockedDecrement( &Formatter->ReferenceCount ) )
	{
		_VERIFY( S_OK == JpdiagsDeleteFormatter( Formatter ) );
	}
}

/*----------------------------------------------------------------------
 *
 * Public
 *
 */

HRESULT JPDIAGCALLTYPE JpdiagCreateFormatter(
	__in PCWSTR FormatTemplate,
	__in_opt PJPDIAG_MESSAGE_RESOLVER Resolver,
	__in_opt DWORD ResolvingFlags,
	__out PJPDIAG_FORMATTER *Result
	)
{
	PFORMATTER Formatter = NULL;
	SIZE_T FormatTemplateCb;
	PWSTR Temp;

	if ( ! JpdiagpIsStringValid( FormatTemplate, 1, MAXWORD, FALSE ) ||
		 ! Result ||
		 ( ResolvingFlags != 0 && ! Resolver ) )
	{
		return E_INVALIDARG;
	}

	FormatTemplateCb = ( wcslen( FormatTemplate ) + 1 ) * sizeof( WCHAR );

	//
	// Allocate space for object and format string.
	//
	Formatter = ( PFORMATTER ) JpdiagpMalloc( 
		sizeof( FORMATTER ) + FormatTemplateCb, TRUE );
	if ( ! Formatter )
	{
		return E_OUTOFMEMORY;
	}

	//
	// Initialize it.
	//
	Temp = ( PWSTR ) 
		( ( PBYTE ) Formatter + sizeof( FORMATTER ) );
	StringCbCopy(
		Temp,
		FormatTemplateCb,
		FormatTemplate );
	Formatter->ReferenceCount = 1;
	Formatter->FormatTemplate = Temp;

	Formatter->Base.Size			= sizeof( FORMATTER );
	Formatter->Base.Dereference		= JpdiagsDereferenceFormatter;
	Formatter->Base.Reference		= JpdiagsReferenceFormatter;
	Formatter->Base.Format			= JpdiagsFormat;
	if ( Resolver )
	{
		Resolver->Reference( Resolver );
		Formatter->Resolver = Resolver;
		Formatter->ResolvingFlags = ResolvingFlags;
	}

	*Result = &Formatter->Base;

	return S_OK;
}