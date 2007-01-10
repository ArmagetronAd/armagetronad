begin
  require "sandbox"
rescue LoadError => e
  STDERR.puts "#{e.class}: #{e}", e.backtrace
end

module Armagetronad
  
  class TOldConf < TConfItemBase
    def self.get_config_item(name)
      ci = find_config_item(name)
      raise(RuntimeError, "Configuration item #{name} does not exist.") if ci.nil?
      ci
    end
    
    def self.update_config_item(name, value)
      ci = get_config_item(name)
      return ci.set(value) if ci.respond_to?(:set)
      ci.read_val(Istringstream.new(value.to_s))
    end
    
    def self.method_missing(name, *args, &block)
      name = name.to_s
      if args.size.zero?
        get_config_item(name)
      elsif args.size == 1
        update_config_item(name.delete("="), args.first)
      # multiple arguments are parsed together into a string
      else
        update_config_item(name, args.join(" "))
      end
    end
  end
  
end
