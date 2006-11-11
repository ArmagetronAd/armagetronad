require "armagetronad/tools"
require "armagetronad/network"
require "armagetronad/render"
require "armagetronad/ui.so"


module ArmagetronAd
  module Ui
    
    # BUG: Error is thrown (Swig::DirectorTypeMismatchException) when help menu is displayed.
    class MenuItemFunction < MenuItemAction
      
      def initialize(menu, name, help, *args, &block)
        super(menu, name, help)
        @block = block
        @args = args
      end
      
      def enter
        @block.call(*@args)
      end
      
    end
    
    class Menu
      
      def add_submenu(title, help=ArmagetronAd::Tools::Output.new)
        submenu = Menu.new(title)
        MenuItemSubmenu.new(self, submenu, help)
        submenu
      end
      
      def add_item_string(title, help, string_pointer, max_length=1024)
        MenuItemString.new(self, title, help, string_pointer, max_length)
      end
      
      def add_item_int(title, help, int_pointer, minimum, maximum)
        MenuItemInt.new(self, title, help, int_pointer, minimum, maximum)
      end
      
      def add_item_toggle(name, help, bool_pointer)
        MenuItemToggle.new(self, name, help, bool_pointer)
      end
      
      def add_item_function(name, help, *args, &block)
        MenuItemFunction.new(self, name, help, *args, &block)
      end
      
    end
  end
end
