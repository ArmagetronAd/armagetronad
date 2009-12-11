/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
  
***************************************************************************

*/

#include "AARuby.h"
#include "aa_config.h"

#ifdef WITH_RUBY
#include <ruby.h>

void AARuby_init_loadpath()
{
	NSBundle *bundle = [NSBundle mainBundle];
    NSString *macosx = [[bundle pathForAuxiliaryExecutable:@"Armagetron Advanced"] stringByDeletingLastPathComponent];
	NSString *path = [NSString stringWithFormat:@"%@/../Resources/ruby", macosx];
    VALUE load_path = rb_gv_get("$:");
	rb_ary_push(load_path, rb_str_new2([[NSString stringWithFormat:@"%@/%s", path, "site_ruby/1.8"] UTF8String]));
	rb_ary_push(load_path, rb_str_new2([[NSString stringWithFormat:@"%@/%s", path, "site_ruby/1.8/powerpc-darwin8.7.0"] UTF8String]));
	rb_ary_push(load_path, rb_str_new2([[NSString stringWithFormat:@"%@/%s", path, "site_ruby"] UTF8String]));
	rb_ary_push(load_path, rb_str_new2([[NSString stringWithFormat:@"%@/%s", path, "1.8"] UTF8String]));
	rb_ary_push(load_path, rb_str_new2([[NSString stringWithFormat:@"%@/%s", path, "1.8/powerpc-darwin8.7.0"] UTF8String]));
}
#endif
