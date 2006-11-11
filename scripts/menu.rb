require "armagetronad"

class MyMenu
  include ArmagetronAd
  def initialize
    @age = Tools::IntPointer.new
    @age.value = 0
    @name = Tools::StringPointer.new
    @alive = Tools::BoolPointer.new
    
    menu = Ui::Menu.new(Tools::Output.new("Test from Ruby"))
    menu.add_item_string(Tools::Output.new("Name"), Tools::Output.new, @name)
    menu.add_item_int(Tools::Output.new("Age"), Tools::Output.new, @age, 0, 150)
    menu.add_item_toggle(Tools::Output.new("Alive?"), Tools::Output.new, @alive)
    # menu.add_item_function(Tools::Output.new("A function"), Tools::Output.new("help for function")) do
    #   puts "function was called!"
    # end
    submenu = menu.add_submenu(Tools::Output.new("A Submenu"))
    
    menu.enter
  end
  
  attr_reader :age, :name, :alive
end

my_menu = MyMenu.new
puts "name  : #{my_menu.name.value}"
puts "age   : #{my_menu.age.value}"
puts "alive?: #{my_menu.alive.value}"
