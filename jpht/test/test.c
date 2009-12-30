#include <cfix.h>
#include <stdlib.h>
#include "hashtable.h"

#define TEST CFIX_ASSERT

static VOID EnumCallback(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID Context
	)
{
	PINT Count = ( PINT ) Context;
	UNREFERENCED_PARAMETER( Hashtable );
	UNREFERENCED_PARAMETER( Entry );
	//wprintf( L"%s=%s\n", Entry->Key, Entry->Value );
	(*Count)++;
}

static VOID EnumRemoveCallback(
	__in PJPHT_HASHTABLE Hashtable,
	__in PJPHT_HASHTABLE_ENTRY Entry,
	__in_opt PVOID Context
	)
{
	PJPHT_HASHTABLE_ENTRY OldEntry;
	UNREFERENCED_PARAMETER( Context );
	JphtRemoveEntryHashtable( Hashtable, Entry->Key, &OldEntry );
	CFIX_ASSERT( OldEntry == Entry );
}


static ULONG HashString(
	__in ULONG_PTR Key
	)
{
	/*PWSTR Str = ( PWSTR ) Key;
	ULONG Hash = 0;
	WCHAR *Wch;

	if ( Str )
	{
		for ( Wch = Str; *Wch; Wch++ )
		{
			Hash += *Wch;
		}
	}

	return Hash;*/

	return ( ULONG ) Key;
}

static BOOLEAN EqualsString (
	__in ULONG_PTR KeyLhs,
	__in ULONG_PTR KeyRhs
	)
{
	PWSTR Lhs = ( PWSTR ) KeyLhs;
	PWSTR Rhs = ( PWSTR ) KeyRhs;

	return ( BOOLEAN ) ( 0 == wcscmp( Lhs, Rhs ) );
}

static PVOID Allocate(
	__in SIZE_T Size 
	)
{
	return malloc( Size );
}

/*++
	Routine Description:
		Free memory previously allocated by JPHT_ALLOCATE_ROUTINE.
--*/
static VOID Free(
	__in PVOID Mem
	)
{
	free( Mem );
}

typedef struct _TEST_ENTRY
{
	JPHT_HASHTABLE_ENTRY Base;
	PCWSTR Value;
} TEST_ENTRY, *PTEST_ENTRY;

static void TestHashtable()
{
	JPHT_HASHTABLE Ht;
	PTEST_ENTRY Old;
	TEST_ENTRY Foo;
	TEST_ENTRY Bar;
	ULONG HtSize;

	Foo.Base.Key = ( ULONG_PTR ) L"Foo";
	Foo.Value = L"Val(Foo)";

	Bar.Base.Key = ( ULONG_PTR ) L"Bar11111";
	Bar.Value = L"Val(Bar)";

	for ( HtSize = 1; HtSize < 10; HtSize++ )
	{
		INT Count = 0;
		TEST( JphtInitializeHashtable(
			&Ht,
			Allocate,
			Free,
			HashString,
			EqualsString,
			HtSize ) );

		TEST( JphtGetEntryHashtable(
			&Ht,
			0 ) == NULL );

		JphtPutEntryHashtable(
			&Ht,
			&Foo.Base,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		TEST( Old == NULL );
		JphtPutEntryHashtable(
			&Ht,
			&Bar.Base,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		TEST( Old == NULL );
		TEST( Ht.Data.EntryCount == 2 );

		JphtEnumerateEntries( &Ht, EnumCallback, &Count );
		TEST( Count == 2 );

		Count = 0;
		JphtEnumerateEntries( &Ht, EnumCallback, &Count );
		TEST( Count == 2 );

		TEST( JphtResize( &Ht, HtSize * 2 ) );
		TEST( Ht.Data.BucketCount == HtSize * 2 );
		TEST( JphtResize( &Ht, HtSize * 3 ) );

		JphtPutEntryHashtable(
			&Ht,
			&Foo.Base,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		TEST( Old == &Foo );

		TEST( ( PTEST_ENTRY ) JphtGetEntryHashtable(
			&Ht,
			Foo.Base.Key ) == &Foo );
		TEST( ( PTEST_ENTRY ) JphtGetEntryHashtable(
			&Ht,
			Bar.Base.Key ) == &Bar );

		JphtRemoveEntryHashtable(
			&Ht,
			Foo.Base.Key,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		TEST( Old == &Foo );
		JphtRemoveEntryHashtable(
			&Ht,
			Bar.Base.Key,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		TEST( Old == &Bar );
		TEST( Ht.Data.EntryCount == 0 );

		JphtRemoveEntryHashtable(
			&Ht,
			0,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		TEST( Old == NULL );

		TEST( JphtGetEntryHashtable(
			&Ht,
			Foo.Base.Key ) == NULL );

		TEST( JphtGetEntryHashtable(
			&Ht,
			Bar.Base.Key ) == NULL );


		JphtPutEntryHashtable(
			&Ht,
			&Foo.Base,
			( PJPHT_HASHTABLE_ENTRY* ) &Old );
		JphtEnumerateEntries( &Ht, EnumRemoveCallback, NULL );

		JphtDeleteHashtable( &Ht );
	}
}

CFIX_BEGIN_FIXTURE( Hashtable )
	CFIX_FIXTURE_ENTRY( TestHashtable )
CFIX_END_FIXTURE()