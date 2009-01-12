require "shellwords"
require "enumerator"

class AA::Version
  
  def initialize()
    @info = initialize_info()
  end
  
  attr_reader :info
  
  def [](key)
    @info[key]
  end
  
  private
  
  # Make a hash from a flat array
  def make_hash(words)
    words.enum_for(:each_slice, 2).inject(Hash.new) { |hash, (key, value)|
      hash[key] = value
      hash
    }
  end
  
  def initialize_info()
    if AA::Config::BUILD_TYPE == :development
      data = version_script(true)
      words = Shellwords.shellwords(data)
      make_hash(words)
    else
      # TODO
    end
  end

  # Call the version script and get its result
  def version_script(verbose=false)
    version_script_path = AA::Config.escape_sh(AA::Config.top_path("batch", "make", "version"))
    flags = verbose ? " -v " : ""
    %x(#{version_script_path} #{flags} #{AA::Config.escape_sh AA::Config::TOP_DIR}).chomp
  end
  
end


module AA::Config
  def self.combine_path_components(base, *components)
    File.join(*([base] + components))
  end
  
  # A path to a file from the top-level armagetronad directory
  def self.top_path(*components)
    combine_path_components(TOP_DIR, *components)
  end
  
  # A path to a file in MacOS/build/
  def self.build_path(*components)
    combine_path_components(BUILD_DIR, *components)
  end
  
  # A path to a file in MacOS/build/Generated/
  def self.generated_path(*components)
    combine_path_components(build_path("Generated"), *components)
  end
  
  # A path to a file in MacOS/build/{Debug, etc}
  def self.configuration_build_path(*components)
    combine_path_components(CONFIGURATION_BUILD_DIR, *components)
  end
  
  # A path to a file in the resource directory of the game.
  def self.package_path(*components)
    combine_path_components(PACKGAGE_RESOURCE_DIR, *components)
  end
  
  # The arch of the build
  def self.arch
    archs = (ENV["ARCHS"] || "i386").split
    if archs.length > 1
      "universal"
    else
      archs.first
    end
  end
  
  # Returns the game version
  def self.version
    VERSION_INFO["VERSION"]
  end

  # escape text to make it useable in a shell script as one “word” (string)
  def self.escape_sh(str)
  	str.to_s.gsub(/(?=[^a-zA-Z0-9_.\/\-\x7F-\xFF\n])/, '\\').gsub(/\n/, "'\n'").sub(/^$/, "''")
  end
    
  # The top-level project directory
  TOP_DIR = (ENV["PROJECT_DIR"] || File.dirname(__FILE__) + "/..") + "/.."
  
  BUILD_DIR = ENV["SYMROOT"] || "build"
  
  CONFIGURATION_BUILD_DIR = ENV["CONFIGURATION_BUILD_DIR"] || (BUILD_DIR + "/Debug")
  
  PRODUCT_NAME = ENV["PRODUCT_NAME"] || "Armagetron Advanced"
  
  DEDICATED = !!PRODUCT_NAME[/dedicated/i]
  
  # The Armagetron Advanced.app, or the Armagetron Advanced Dedicated directory
  PACKGAGE_RESOURCE_DIR_BASE = [
    CONFIGURATION_BUILD_DIR,
    DEDICATED ? nil : PRODUCT_NAME + (ENV["WRAPPER_SUFFIX"] || ".app")
  ].compact.join("/")
  
  # Where all the game data should go
  PACKGAGE_RESOURCE_DIR = [
    PACKGAGE_RESOURCE_DIR_BASE,
     DEDICATED ? nil : "Contents/Resources"
  ].compact.join("/")
    
  BUILD_TYPE = [top_path(".svn"), top_path(".bzr")].any? { |f| File.exists?(f) } ? :development : :release
    
  PROGRAM_SHORT_NAME = DEDICATED ? "armagetronad-dedicated" : "armagetronad"
  
  VERSION_INFO = AA::Version.new
end
