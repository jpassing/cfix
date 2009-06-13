/*----------------------------------------------------------------------
 * Copyright:
 *		2008, Johannes Passing (passing at users.sourceforge.net)
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
 

/*++
	Simple adder class - this is the class that will be tested by
	the unit test.
--*/

class Adder
{
private:
	int sum;

	Adder( const Adder& );
	const Adder& operator = ( const Adder& );

public:
	Adder();

	/*++
		Add to the accumulator.
	--*/
	void Add( int value );
	
	/*++
		Get the current sum.
	--*/
	int GetSum();

	/*++
		Reset sum.
	--*/
	void Reset();
};

