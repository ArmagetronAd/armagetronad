require "armagetronad/tools.so"

# Make the pointer classes more consistent
%w[IntPointer BoolPointer FloatPointer StringPointer].each do |klass|
  ArmagetronAd::Tools.const_get(klass).class_eval do
    alias_method(:value=, :assign)
  end
end

# Set new constants for TSettingItem«type» and TConfItem«type», removing
# the T prefix
types = %w[String Int Bool Float]
%w[ConfItem SettingItem].each do |class_name_base|
  types.each do |t|
    class_name = class_name_base + t
    old_class = ArmagetronAd::Tools.const_get("T" + class_name)
    ArmagetronAd::Tools.const_set(class_name, old_class)
  end
end
