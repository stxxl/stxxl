/***************************************************************************
                          test.cpp  -  description
                             -------------------
    begin                : Sam Feb 12 2005
    copyright            : (C) 2005 by Thomas Nowak
    email                : t.nowak@imail.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

 #include "btnode.h"
 #include "my_type.h"


 class my_class : public stxxl::map_internal::ref_counter<my_class>
 {
	my_class() {};
 };
 
 int main()
 {
	 stxxl::map_internal::btnode<key_type,data_type,cmp2,4096,true,0> m;
	 stxxl::map_internal::smart_ptr<my_class> sp_ptr;
	 int i = 0;
	 i++;
 };
 
	