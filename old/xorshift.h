/*
This file is part of podds.

podds is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

podds is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with podds.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __XORSHIFT_H__
#define __XORSHIFT_H__

#include <stdint.h>

uint32_t xorshift32_rand(uint32_t *);
uint32_t xorshift32_randint(uint32_t *, uint32_t);

#endif // __XORSHIFT_H__
