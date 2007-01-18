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
    ScriptAI.new
  end
end

Armagetronad::GSimpleAIFactory.set(ScriptAIFactory.new)
