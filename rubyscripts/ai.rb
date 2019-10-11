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
	ObjectSpace.define_finalizer( ret, lambda { puts "ai collected" } )
	ret
  end
end

factory = ScriptAIFactory.new
Armagetronad::GSimpleAIFactory.set(factory)
ObjectSpace.define_finalizer( factory, lambda { puts "factory collected" } )
# keep the factory variable, it prevents the factory from getting collected.
# the AIs get called in short intervals and should survive because of that
# (ugly and not to be kept that way)
puts "ai initialized"
