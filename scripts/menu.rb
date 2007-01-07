class MyMenu
  include Armagetronad
  def initialize
    menu = UMenu.new(TOutput.new("Test from Ruby"))
    
    submenu = UMenu.new(TOutput.new("Submenu"))
    UMenuItemSubmenu.new(menu, submenu, TOutput.new("helo me"))
    
    
    menu.enter
  end
end

MyMenu.new