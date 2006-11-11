begin
  require "armagetronad"
  require "sandbox"
rescue LoadError => e
  STDERR.puts "#{e.class}: #{e}", e.backtrace
end
