/*----------------------------------------------------------------------
 * Purpose:
 *		Hashtable implementatoin. The implementation follows
 *		the design of an open hashtable.
 *
 * Copyright:
 *		Johannes Passing (johannes.passing@googlemail.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "stdafx.h"
#include "hashtable.h"
#include <windows.h>

#include "list.h"
#include <crtdbg.h>
#define ASSERT _ASSERTE

/*----------------------------------------------------------------------
 *
 * Implementation.
 *
 */

BOOLEAN JphtInitializeHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in JPHT_ALLOCATE_ROUTINE Allocate,
	__in JPHT_FREE_ROUTINE Free,
	__in JPHT_HASH_ROUTINE Hash,
	__in JPHT_EQUALS_ROUTINE Equals,
	__in ULONG InitialBucketCount
	)
{
	ULONG Index;
	__int64 SizeRequired;

	ASSERT( Hashtable );
	ASSERT( Allocate );
	ASSERT( Free );
	ASSERT( Hash );
	ASSERT( Equals );
	ASSERT( InitialBucketCount > 0 );

	Hashtable->Routines.Allocate	= Allocate;
	Hashtable->Routines.Free		= Free;
	Hashtable->Routines.Equals		= Equals;
	Hashtable->Routines.Hash		= Hash;

	Hashtable->Data.BucketCount		= InitialBucketCount;
	Hashtable->Data.EntryCount		= 0;

	//
	// Allocate space for buckets.
	//
	SizeRequired = InitialBucketCount * sizeof( JPHT_HASHTABLE_BUCKET );
	if ( SizeRequired > ( SIZE_T ) -1 )
	{
		//
		// Overflow.
		//
		return FALSE;
	}
	Hashtable->Data.Buckets = 
		Hashtable->Routines.Allocate( ( SIZE_T ) SizeRequired );
	if ( Hashtable->Data.Buckets == NULL )
	{
		return FALSE;
	}

	//
	// Initialize all list heads.
	//
	for ( Index = 0; Index < Hashtable->Data.BucketCount; Index++ )
	{
		InitializeListHead( &Hashtable->Data.Buckets[ Index ].EntryListHead );
	}

	return TRUE;
}

VOID JphtDeleteHashtable(
	__in PJPHT_HASHTABLE Hashtable 
	)
{
	ASSERT( Hashtable );
	ASSERT( Hashtable->Data.EntryCount == 0 );

	Hashtable->Routines.Free( Hashtable->Data.Buckets );

#ifdef DBG
	Hashtable->Data.BucketCount = 0xcccccccc;
	Hashtable->Data.EntryCount = 0xcccccccc;
	Hashtable->Data.Buckets = NULL;
#endif
}


VOID JphtPutEntryHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__out_opt PJPHT_HASHTABLE_ENTRY *OldEntry
	)
{
	ULONG BucketIndex;
	PJPHT_HASHTABLE_BUCKET Bucket;
	PLIST_ENTRY ListEntry;
	PJPHT_HASHTABLE_ENTRY ExistingEntry;

	ASSERT( Hashtable );
	ASSERT( Entry );
	ASSERT( OldEntry );

	//
	// Get bucket.
	//
	BucketIndex = Hashtable->Routines.Hash( Entry->Key ) 
		% Hashtable->Data.BucketCount;
	Bucket = &Hashtable->Data.Buckets[ BucketIndex ];

	//
	// Check if key is contained in list. If yes, remove the previous 
	// entry.
	//
	*OldEntry = NULL;

	ListEntry = Bucket->EntryListHead.Flink;
	
	while ( ListEntry != &Bucket->EntryListHead )
	{
		ExistingEntry = CONTAINING_RECORD( 
			ListEntry, JPHT_HASHTABLE_ENTRY, ListEntry ); 
		if ( Hashtable->Routines.Equals( Entry->Key, ExistingEntry->Key ) )
		{
			//
			// Entry found, remove and free.
			//
			RemoveEntryList( &ExistingEntry->ListEntry );
			*OldEntry = ExistingEntry;
			Hashtable->Data.EntryCount--;
		}
		ListEntry = ListEntry->Flink;
	}

	//
	// Now insert new entry.
	//
	InsertHeadList( &Bucket->EntryListHead, &Entry->ListEntry );
	Hashtable->Data.EntryCount++;
}

PJPHT_HASHTABLE_ENTRY JphtGetEntryHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in ULONG_PTR Key 
	)
{
	PJPHT_HASHTABLE_BUCKET Bucket;
	PLIST_ENTRY ListEntry;
	PJPHT_HASHTABLE_ENTRY Entry = NULL;


	ASSERT( Hashtable );

	//
	// Get bucket.
	//
	Bucket = &Hashtable->Data.Buckets[
		Hashtable->Routines.Hash( Key ) % Hashtable->Data.BucketCount ];

	//
	// Search for key in list.
	//
	ListEntry = Bucket->EntryListHead.Flink;
	
	while ( ListEntry != &Bucket->EntryListHead )
	{
		Entry = CONTAINING_RECORD( 
			ListEntry, JPHT_HASHTABLE_ENTRY, ListEntry ); 
		if ( Hashtable->Routines.Equals( Entry->Key, Key ) )
		{
			//
			// Entry found.
			//
			return Entry;
		}
		ListEntry = ListEntry->Flink;
	}

	return NULL;
}

VOID JphtRemoveEntryHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in ULONG_PTR Key,
	__out_opt PJPHT_HASHTABLE_ENTRY *OldEntry
	)
{
	PJPHT_HASHTABLE_BUCKET Bucket;
	PLIST_ENTRY ListEntry;
	PJPHT_HASHTABLE_ENTRY Entry = NULL;

	ASSERT( Hashtable );
	ASSERT( OldEntry );

	//
	// Get bucket.
	//
	Bucket = &Hashtable->Data.Buckets[
		Hashtable->Routines.Hash( Key ) % Hashtable->Data.BucketCount ];

	//
	// Search for key in list.
	//
	if ( OldEntry )
	{
		*OldEntry = NULL;
	}

	ListEntry = Bucket->EntryListHead.Flink;
	
	while ( ListEntry != &Bucket->EntryListHead )
	{
		Entry = CONTAINING_RECORD( 
			ListEntry, JPHT_HASHTABLE_ENTRY, ListEntry ); 
		if ( Hashtable->Routines.Equals( Entry->Key, Key ) )
		{
			//
			// Entry found.
			//
			if ( OldEntry )
			{
				*OldEntry = Entry;
			}

			RemoveEntryList( ListEntry );
			Hashtable->Data.EntryCount--;
			break;
		}
		ListEntry = ListEntry->Flink;
	}
}

VOID JphtEnumerateEntries(
	__in PJPHT_HASHTABLE Hashtable,
	__in JPHT_ENUM_CALLBACK Callback,
	__in_opt PVOID Context
	)
{
	ULONG BucketIndex;

	ASSERT( Hashtable );
	ASSERT( Callback );

	//
	// Iterate over all buckets.
	//
	for ( BucketIndex = 0; BucketIndex < Hashtable->Data.BucketCount; BucketIndex++ )
	{
		PJPHT_HASHTABLE_BUCKET Bucket = &Hashtable->Data.Buckets[ BucketIndex ];
		PLIST_ENTRY ListEntry;
		PJPHT_HASHTABLE_ENTRY Entry = NULL;

		//
		// Iterate over all entries in bucket.
		//
		ListEntry = Bucket->EntryListHead.Flink;

		while ( ListEntry != &Bucket->EntryListHead )
		{
			PLIST_ENTRY NextEntry;
			Entry = CONTAINING_RECORD( 
				ListEntry, JPHT_HASHTABLE_ENTRY, ListEntry ); 

			//
			// Callback is free to delete entry, so do not touch 
			// ListEntry after callback return.
			//
			NextEntry = ListEntry->Flink;

			( Callback )( Hashtable, Entry, Context );

			ListEntry = NextEntry;
		}
	}
}

BOOLEAN JphtResize(
	__in PJPHT_HASHTABLE Hashtable,
	__in ULONG NewBucketCount
	)
{
	ULONG OldBucketCount;
	PJPHT_HASHTABLE_BUCKET OldBuckets;
	SIZE_T SizeRequired;
	PVOID AllocatedMemory;
	ULONG BucketIndex;

	ASSERT( Hashtable );
	ASSERT( NewBucketCount );

	//
	// Save pointer to old buckets list
	//
	OldBuckets = Hashtable->Data.Buckets;
	OldBucketCount = Hashtable->Data.BucketCount;

	//
	// Allocate space for buckets.
	//
	SizeRequired = NewBucketCount * sizeof( JPHT_HASHTABLE_BUCKET );
	if ( SizeRequired > ( SIZE_T ) -1 )
	{
		//
		// Overflow.
		//
		return FALSE;
	}

	AllocatedMemory =  Hashtable->Routines.Allocate( SizeRequired );
	if ( AllocatedMemory == NULL )
	{
		return FALSE;
	}

	//
	// Now swap pointers.
	//
	Hashtable->Data.BucketCount = NewBucketCount;
	Hashtable->Data.Buckets = ( PJPHT_HASHTABLE_BUCKET ) AllocatedMemory;
	Hashtable->Data.EntryCount = 0;


	//
	// Initialize all list heads of new list.
	//
	for ( BucketIndex = 0; BucketIndex < Hashtable->Data.BucketCount; BucketIndex++ )
	{
		InitializeListHead( &Hashtable->Data.Buckets[ BucketIndex ].EntryListHead );
	}

	//
	// Copy all entries.
	//
	// Iterate over all buckets - note that we use the old list.
	//
	for ( BucketIndex = 0; BucketIndex < OldBucketCount; BucketIndex++ )
	{
		PJPHT_HASHTABLE_BUCKET Bucket = &OldBuckets[ BucketIndex ];
		PLIST_ENTRY ListEntry;
		PJPHT_HASHTABLE_ENTRY Entry = NULL;
		PJPHT_HASHTABLE_ENTRY OldEntry = NULL;

		//
		// Iterate over all entries in bucket.
		//
		ListEntry = Bucket->EntryListHead.Flink;

		while ( ListEntry != &Bucket->EntryListHead )
		{
			PLIST_ENTRY NextEntry = NULL;
			Entry = CONTAINING_RECORD( 
				ListEntry, JPHT_HASHTABLE_ENTRY, ListEntry ); 

			//
			// Insert to new list - note that this modifies
			// Entry->ListEntry.
			//
			NextEntry = ListEntry->Flink;
			JphtPutEntryHashtable(
				Hashtable,
				Entry,
				&OldEntry );

			ASSERT( OldEntry == NULL );

			ListEntry = NextEntry;
		}
	}

	//
	// Free old bucket list.
	//
	Hashtable->Routines.Free( OldBuckets );

	return TRUE;
}
