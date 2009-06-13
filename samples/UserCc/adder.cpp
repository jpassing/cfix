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
 
#include "stdafx.h"
#include "adder.h"

Adder::Adder() : sum( 0 )
{
}

void Adder::Add( int value )
{
	int newSum = this->sum + value;
		
	if ( value > 0 && newSum <= this->sum )
	{
		//
		// Overflow occured.
		//
		throw std::overflow_error( "integer overflow" );
	}
	else if ( value < 0 && newSum >= this->sum )
	{
		//
		// Underflow occured.
		//
		throw std::underflow_error( "integer overflow" );
	}
	else
	{
		this->sum = newSum;
	}
}

int Adder::GetSum()
{
	return this->sum;
}

void Adder::Reset()
{
	this->sum = 0;
}
