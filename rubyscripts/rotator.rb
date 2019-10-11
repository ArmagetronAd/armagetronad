require "armagetronad"

class Rotator
  
  MAPS = %w[Anonymous/polygon/regular/square-1.0.1.aamap.xml
            Anonymous/polygon/regular/40-gon-0.1.1.aamap.xml
            Anonymous/polygon/regular/diamond-1.0.2.aamap.xml
            Your_mom/clever/inaktek-0.7.2.aamap.xml
            Your_mom/clever/repeat-0.3.2.aamap.xml
            Z-Man/fortress/sumo_4x4-0.1.1.aamap.xml
            Z-Man/fortress/zonetest-0.1.0.aamap.xml]
  
  def initialize
    @map = 0
  end
  
  def round_event
    map = next_map()
    ArmagetronAd::Tools::ConfItemBase.load_line("say Get ready for #{File.basename(map)}!")
    ArmagetronAd::Tools::ConfItemBase.load_line("map_file #{map}")
  end
  
  def match_event
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
  rotator.match_event
end
