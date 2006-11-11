require "armagetronad"

class Rotator
  
  MAPS = %w[Your_mom/clever/repeat-0.3.2.aamap.xml
            Your_mom/clever/inaktek-0.7.2.aamap.xml
            Anonymous/polygon/regular/40-gon-0.1.1.aamap.xml
            Anonymous/polygon/regular/diamond-1.0.2.aamap.xml
            Z-Man/fortress/zonetest-0.1.0.aamap.xml]
  
  def initialize
    @rounds_played = 0
    @matches_played = 0
    @map = 0
  end
  
  def round_event
    @rounds_played += 1
    puts "rounds_played = #{@rounds_played}"    
    ArmagetronAd::Tools::ConfItemBase.load_line("map_file #{next_map()}")
  end
  
  def match_event
    @matches_played += 1
    puts "matches_played = #{@matches_played}"
  end
  
  def next_map
    @map += 1
    # ring around
    if @map > MAPS.length - 1
      @map = 0
    end
    MAPS[@map]      
  end
  
end

rotator = Rotator.new

ArmagetronAd::Tron::RoundEvent.new do
  rotator.round_event
end

ArmagetronAd::Tron::MatchEvent.new do
  round_event.match_event
end
