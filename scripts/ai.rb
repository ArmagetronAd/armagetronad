class ScriptAI < Armagetronad::GSimpleAI
  include Armagetronad
  
  MIN_THINK_TIME = 0.1
  MAX_THINK_TIME = 0.1
  
  def do_think
    cycle = object()
    front = GSensor.new(cycle, cycle.position, cycle.direction)
    front.detect(cycle.speed * (MIN_THINK_TIME + MAX_THINK_TIME))
    
    if front.ehit
      cycle.turn(-front.lr)
    else
      cycle.turn(front.lr)
    end
    
    MAX_THINK_TIME
  end
end

class ScriptAIFactory < Armagetronad::GSimpleAIFactory
  def do_create
    ret = ScriptAI.new
	ObjectSpace.define_finalizer( ret, slambda { puts "ai collected" } )
	ret
  end
end

factory = ScriptAIFactory.new
Armagetronad::GSimpleAIFactory.set(factory)
ObjectSpace.define_finalizer( factory, slambda { puts "factory collected" } )
#factory = 1
puts "ai"