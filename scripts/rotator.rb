require "armagetronad"

class Rotator
  def initialize
    @rounds_played = 0
    @matches_played = 0
  end
  
  def round_event
    @rounds_played += 1
    puts "rounds_played = #{@rounds_played}"
    
    # even rounds are faster
    if (@rounds_played % 2).zero?
      ArmagetronAd::Tools::ConfItemBase.load_line("speed_factor 4")
    else
      ArmagetronAd::Tools::ConfItemBase.load_line("speed_factor 0")
    end
  end
  
  def match_event
    @matches_played += 1
    puts "matches_played = #{@matches_played}"
  end
end

rotator = Rotator.new

ArmagetronAd::Tron::RoundEvent.new do
  rotator.round_event
end

ArmagetronAd::Tron::MatchEvent.new do
  round_event.match_event
end
