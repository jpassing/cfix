/*----------------------------------------------------------------------
 * Purpose:
 *		Hashtable routines.
 *
 *		Note that no internal locking is performed, i.e. while 
 *		concurrent read access are allowed, the user must avoid
 *		performing concurrent modifications on the hashtable.
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

#pragma once

typedef struct _JPHT_HASHTABLE_BUCKET
{
	LIST_ENTRY EntryListHead;
} JPHT_HASHTABLE_BUCKET, *PJPHT_HASHTABLE_BUCKET;

typedef struct _JPHT_HASHTABLE_ENTRY
{
	ULONG_PTR Key;
	LIST_ENTRY ListEntry;
} JPHT_HASHTABLE_ENTRY, *PJPHT_HASHTABLE_ENTRY;

/*++
	Routine Description:
		Calculate hash for given key.
--*/
typedef ULONG ( * JPHT_HASH_ROUTINE ) (
	__in ULONG_PTR Key
	);

/*++
	Routine Description:
		Test two keys for equality.
--*/
typedef BOOLEAN ( * JPHT_EQUALS_ROUTINE ) (
	__in ULONG_PTR KeyLhs,
	__in ULONG_PTR KeyRhs
	);

/*++
	Routine Description:
		Allocate memory.

	Return Value;
		Pointer to valid, uninitialized memory on success.
		NULL on failure.
--*/
typedef PVOID ( * JPHT_ALLOCATE_ROUTINE ) (
	__in SIZE_T Size 
	);

/*++
	Routine Description:
		Free memory previously allocated by JPHT_ALLOCATE_ROUTINE.
--*/
typedef VOID ( * JPHT_FREE_ROUTINE ) (
	__in PVOID Mem
	);

typedef struct _JPHT_HASHTABLE
{
	struct
	{
		JPHT_ALLOCATE_ROUTINE Allocate;
		JPHT_FREE_ROUTINE Free;
		JPHT_HASH_ROUTINE Hash;
		JPHT_EQUALS_ROUTINE Equals;
	} Routines;

	struct
	{
		ULONG EntryCount;
		ULONG BucketCount;
		PJPHT_HASHTABLE_BUCKET Buckets;
	} Data;
} JPHT_HASHTABLE, *PJPHT_HASHTABLE;

/*++
	Routine Description:
		Determine number of entries in hashtable.
--*/
#define JphtGetEntryCountHashtable( ht ) \
	( ( ht )->Data.EntryCount )

/*++
	Routine Description:
		Determine number of buckets in hashtable.
--*/
#define JphtGetBucketCountHashtable( ht ) \
	( ( ht )->Data.BucketCount )

/*++
	Routine Description:
		Initialize hashtable structure and allocate memory
		for holding buckets.

		The Allocate routine will be called.

	Return Value:
		TRUE on success.
		FALSE if memory allocation failed.
--*/
BOOLEAN JphtInitializeHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in JPHT_ALLOCATE_ROUTINE Allocate,
	__in JPHT_FREE_ROUTINE Free,
	__in JPHT_HASH_ROUTINE Hash,
	__in JPHT_EQUALS_ROUTINE Equals,
	__in ULONG InitialBucketCount
	);

/*++
	Routine Description:
		Free resources associated with this hashtable.

		Note: The hashtable is assumed to be empty! The caller has
		to make sure that all entries have previously been removed
		and freed.

		To remove all entries from the hashtable, you may use 
		JphtEnumerateEntries and remove each entry from within the
		callback. It is important that the caller frees entries itself
		as only the caller knows the appropriate cleanup meachanism.

		The Free routine will be called.

	Return Value:
		TRUE on success.
		FALSE if memory allocation failed.
--*/
VOID JphtDeleteHashtable(
	__in PJPHT_HASHTABLE Hashtable 
	);

/*++
	Routine Description:
		Put entry into hashtable. If an entry with the same
		key existed, it is returned vis OldEntry. The caller
		must free the associated resources.

		The entry is inserted as-is, i.e. the memory must remain
		valid until the entry is explicitly removed from the 
		hashtable.

		The Hash and Equals routines will be called.

	Parameters:
		Hashtable
		Entry		- entry to insert.
		OldEntry	- overwritten entry if any.
--*/
VOID JphtPutEntryHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__out_opt PJPHT_HASHTABLE_ENTRY *OldEntry
	);

/*++
	Routine Description:
		Retrieve entry from hashtable.

		The Hash and Equals routines will be called.

	Parameters:
		Hashtable
		Key

	Return Value:
		Entry or NULL of not found.
--*/
PJPHT_HASHTABLE_ENTRY JphtGetEntryHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in ULONG_PTR Key 
	);


/*++
	Routine Description:
		Remove entry from hashtable. If found, the entry is returned
		via OldEntry. The caller must free the associated resources.

		The Hash and Equals routines will be called.

	Parameters:
		Hashtable
		Entry		- entry to insert.
		OldEntry	- Removed entry if any.
--*/
VOID JphtRemoveEntryHashtable(
	__in PJPHT_HASHTABLE Hashtable,
	__in ULONG_PTR Key,
	__out_opt PJPHT_HASHTABLE_ENTRY *OldEntry
	);


/*++
	Routine Description:
		Called by JphtEnumerateEntries for each entry.

		Note that it is allowed to call 
		JphtRemoveEntryHashtable( Hashtable, Entry->Key, ... )
		from within this callback.

	Parameters:
		Hashtable	- HT being enumerated.
		Entry		- Current entry.
		Context		- Caller supplied context passed to 
					  JphtEnumerateEntries.
--*/
typedef VOID ( * JPHT_ENUM_CALLBACK ) (
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID Context
	);

/*++
	Routine Description:
		Enumerate all entries in hashtable and call Callback
		for each item encountered.

	Parameters
		Callback	- Routine to call.
		Context		- Caller-supplied context passed to Callback.
--*/	
VOID JphtEnumerateEntries(
	__in PJPHT_HASHTABLE Hashtable,
	__in JPHT_ENUM_CALLBACK Callback,
	__in_opt PVOID Context
	);

/*++
	Routine Description:
		Resize and rehash hashtable.

		The Free and Allocate routines will be called.
	
	Parameters:
		Hashtable
		NewBucketCount	- new number of entries/buckets.

	Return Value:
		TRUE on success.
		FALSE if memory allocation failed. The hashtable is left
				in the previous state.
--*/
BOOLEAN JphtResize(
	__in PJPHT_HASHTABLE Hashtable,
	__in ULONG NewBucketCount
	);
