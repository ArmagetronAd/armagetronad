#include "AARuby.h"
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
