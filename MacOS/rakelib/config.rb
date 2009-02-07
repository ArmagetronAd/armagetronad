require "shellwords"
require "enumerator"
require "yaml"

module AA
class Version
  def initialize(version_file)
    @version_file = version_file
  end
  
  attr_reader :info
  attr_reader :version_file
  
  def [](key)
    @info[key]
  end
  
  def initialize_info
    if !File.directory?(AA::CONFIG.generated_path)
      mkdir_p(AA::CONFIG.generated_path)
    end
    
    if File.exist?(@version_file)
      @info = File.open(@version_file) { |f| YAML.load(f) }
    else
      @info = generate_version()
      File.open(@version_file, "w") { |f| YAML.dump(@info, f) }
    end
  end
  
  private
  
  # Make a hash from a flat array
  def make_hash(words)
    words.enum_for(:each_slice, 2).inject(Hash.new) { |hash, (key, value)|
      hash[key] = value
      hash
    }
  end
  
  def generate_version()
    if AA::CONFIG.build_type == :development
      data = version_script(true)
      words = Shellwords.shellwords(data)
      make_hash(words)
    else
      # TODO: get version from version.h
    end
  end

  # Call the version script and get its result
  def version_script(verbose=false)
    puts "Running batch/make/version…"
    
    version_script_path = AA::Config.escape_sh(AA::CONFIG.top_path("batch", "make", "version"))
    flags = verbose ? " -v " : ""
    %x(#{version_script_path} #{flags} #{AA::Config.escape_sh AA::CONFIG.top_dir}).chomp
  end
  
end


class AA::Config
  def initialize
    # The top-level project directory
    @top_dir = (ENV["PROJECT_DIR"] || File.dirname(__FILE__) + "/..") + "/.."
    @build_dir = ENV["SYMROOT"] || "build"
    @configuration_build_dir = ENV["CONFIGURATION_BUILD_DIR"] || (@build_dir + "/Debug")
    @product_name = ENV["PRODUCT_NAME"] || "Armagetron Advanced"
    @dedicated = !!@product_name[/dedicated/i]
    
    # The Armagetron Advanced.app, or the Armagetron Advanced Dedicated directory
    @packgage_resource_dir_base = [
      @configuration_build_dir,
      @dedicated ? nil : @product_name + (ENV["WRAPPER_SUFFIX"] || ".app")
    ].compact.join("/")

    # Where all the game data should go
    @packgage_resource_dir = [
      @packgage_resource_dir_base,
       @dedicated ? nil : "Contents/Resources"
    ].compact.join("/")

    @build_type = [top_path(".svn"), top_path(".bzr")].any? { |f| File.exists?(f) } ? :development : :release
    @program_short_name = @dedicated ? "armagetronad-dedicated" : "armagetronad"
    
    @version_info = AA::Version.new(generated_path("version.yaml"))
  end
  
  attr_reader :top_dir
  attr_reader :build_dir
  attr_reader :configuration_build_dir
  attr_reader :product_name
  attr_reader :packgage_resource_dir_base
  attr_reader :packgage_resource_dir
  attr_reader :build_type
  attr_reader :program_short_name
  
  attr_accessor :version_info
  
  def dedicated?
    @dedicated
  end
  
  def combine_path_components(base, *components)
    File.join(*([base] + components))
  end
  
  # A path to a file from the top-level armagetronad directory
  def top_path(*components)
    combine_path_components(@top_dir, *components)
  end
  
  # A path to a file in MacOS/build/
  def build_path(*components)
    combine_path_components(@build_dir, *components)
  end
  
  # A path to a file in MacOS/build/Generated/
  def generated_path(*components)
    combine_path_components(build_path("Generated"), *components)
  end
  
  # A path to a file in MacOS/build/{Debug, etc}
  def configuration_build_path(*components)
    combine_path_components(@configuration_build_dir, *components)
  end
  
  # A path to a file in the resource directory of the game.
  def package_path(*components)
    combine_path_components(@packgage_resource_dir, *components)
  end
  
  # The arch of the build
  def arch
    archs = (ENV["ARCHS"] || "i386").split
    if archs.length > 1
      "universal"
    else
      archs.first
    end
  end
  
  # Returns the game version
  def version
    @version_info["VERSION"]
  end

  # escape text to make it useable in a shell script as one “word” (string)
  def self.escape_sh(str)
  	str.to_s.gsub(/(?=[^a-zA-Z0-9_.\/\-\x7F-\xFF\n])/, '\\').gsub(/\n/, "'\n'").sub(/^$/, "''")
  end
end
end
